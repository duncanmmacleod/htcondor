//TEMPTEMP -- things to still do:
// * maybe change the syntax for backwards compatibility
// * add DEFER to rescue DAG
// * maybe retry limit
// * version history
// * actual documentation
// * finish the automated test
//TEMPTEMP -- enhancements (probably make a second ticket)
// * separate struct for defer members
// * retry limit (if not done the first time)
// * move _post from Script to ScriptQ
// * order nodes by _nextRunTime in _waitingQueue?
/***************************************************************
 *
 * Copyright (C) 1990-2007, Condor Team, Computer Sciences Department,
 * University of Wisconsin-Madison, WI.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License.  You may
 * obtain a copy of the License at
 * 
 *    http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************/


#include "condor_common.h"

#include "script.h"
#include "util.h"
#include "job.h"

#include "dag.h"

ScriptQ::ScriptQ( Dag* dag ) : 
	_scriptDeferredCount	(0)
{
	_dag = dag;
	_numScriptsRunning = 0;

    _scriptPidTable = new HashTable<int,Script*>( 127, &hashFuncInt );
    _waitingQueue = new Queue<Script*>();

    if( _scriptPidTable == NULL || _waitingQueue == NULL ) {
        EXCEPT( "ERROR: out of memory!");
    }

 	// register daemonCore reaper for PRE/POST script completion
    _scriptReaperId =
		daemonCore->Register_Reaper( "PRE/POST Script Reaper",
									 (ReaperHandlercpp)&ScriptQ::ScriptReaper,
									 "ScriptQ::ScriptReaper", this );
}

ScriptQ::~ScriptQ()
{
	// should we un-register the daemonCore reaper here?
	delete _scriptPidTable;
	delete _waitingQueue;
};

// run script if possible, otherwise insert it into the waiting queue
int
ScriptQ::Run( Script *script )
{
	//TEMP -- should ScriptQ object know whether it's pre or post?
	const char *prefix = script->_post ? "POST" : "PRE";

	bool deferScript = false;

		// Defer PRE scripts if the DAG is halted (we need to go ahead
		// and run POST scripts so we don't "waste" the fact that the
		// job completed).  (Allow PRE script for final node, though.)
	if ( _dag->IsHalted() && !script->_post &&
				!script->GetNode()->GetFinal() ) {
		debug_printf( DEBUG_DEBUG_1,
					"Deferring %s script of node %s because DAG is halted\n",
					prefix, script->GetNodeName() );
		deferScript = true;
	}

		// Defer the script if we've hit the max PRE/POST scripts
		// running limit.
		//TEMP -- the scriptQ object should really know the max scripts
		// limit, instead of getting it from the Dag object.  wenger 2015-03-18
	int maxScripts =
		script->_post ? _dag->_maxPostScripts : _dag->_maxPreScripts;
	if ( maxScripts != 0 && _numScriptsRunning >= maxScripts ) {
			// max scripts already running
		debug_printf( DEBUG_DEBUG_1, "Max %s scripts (%d) already running; "
					  "deferring %s script of Job %s\n", prefix, maxScripts,
					  prefix, script->GetNodeName() );
		deferScript = true;
	}

	if ( deferScript ) {
		_scriptDeferredCount++;
		_waitingQueue->enqueue( script );
		return 0;
	}

	debug_printf( DEBUG_NORMAL, "Running %s script of Node %s...\n",
				  prefix, script->GetNodeName() );
	_dag->GetJobstateLog().WriteScriptStarted( script->GetNode(),
				script->_post );
	if( int pid = script->BackgroundRun( _scriptReaperId,
				_dag->_dagStatus, _dag->NumNodesFailed() ) ) {
		_numScriptsRunning++;
		_scriptPidTable->insert( pid, script );
		debug_printf( DEBUG_DEBUG_1, "\tspawned pid %d: %s\n", pid,
					  script->_cmd );
		return 1;
	}
	// BackgroundRun() returned pid 0
	debug_printf( DEBUG_NORMAL, "  error: daemonCore->Create_Process() "
				  "failed; %s script of Job %s failed\n", prefix,
				  script->GetNodeName() );

	// Putting this code here fixes PR 906 (Missing PRE or POST script
	// causes DAGMan to hang); also, without this code a node for which
	// the script spawning fails will permanently add to the running
	// script count, which will throw off the maxpre/maxpost throttles.
	// wenger 2007-11-08
	const int returnVal = 1<<8;
	if( ! script->_post ) {
		_dag->PreScriptReaper( script->GetNode(), returnVal );
	} else {
		_dag->PostScriptReaper( script->GetNode(), returnVal );
	}

	return 0;
}

int
ScriptQ::RunWaitingScripts( bool justOne )
{
	int scriptsRun = 0;
	time_t now = time( NULL );

	//TEMPTEMP -- better names for maxNum, curNum??
		// Note:  We do NOT want to re-evaluate maxNum each time through
		// the loop!
	int maxNum = _waitingQueue->Length();
	for ( int curNum = 0; curNum < maxNum; ++curNum ) {
		Script *script;
		_waitingQueue->dequeue( script );
		ASSERT( script != NULL );
		if ( script->_nextRunTime != 0 && script->_nextRunTime > now ) {
				// Deferral time is not yet up -- put it back into the queue.
			_waitingQueue->enqueue( script );
		} else {
				// Try to run the script.  Note:  Run() takes care of
				// checking for halted state and maxpre/maxpost.  TEMPTEMP -- make sure that's right!
			if ( Run( script ) ) {
				++scriptsRun;
				if ( justOne ) {
					break;
				}
			} else {
					// We're halted or we hit maxpre/maxpost, so don't
					// try to run any more scripts.
				//TEMPTEMP -- make sure this works right!
debug_printf( DEBUG_DEBUG_1, "DIAG 1010\n" );//TEMPTEMP
				break;
			}
		}
	}

	debug_printf( DEBUG_DEBUG_1, "Started %d deferred scripts\n", scriptsRun );
	return scriptsRun;
}

int
ScriptQ::NumScriptsRunning()
{
	return _numScriptsRunning;
}

int
ScriptQ::ScriptReaper( int pid, int status )
{
	Script* script = NULL;

	// get the Script* that corresponds to this pid
	_scriptPidTable->lookup( pid, script );
	ASSERT( script != NULL );

	_scriptPidTable->remove( pid );
	_numScriptsRunning--;

	if ( pid != script->_pid ) {
		EXCEPT( "Reaper pid (%d) does not match expected script pid (%d)!",
					pid, script->_pid );
	}

	// Check to see if we should re-run this later.
	if ( script->_deferStatus != SCRIPT_DEFER_STATUS_NONE &&
				WEXITSTATUS( status ) == script->_deferStatus ) {
		++_scriptDeferredCount;
		script->_nextRunTime = time( NULL ) + script->_deferTime;
		_waitingQueue->enqueue( script );
		const char *prefix = script->_post ? "POST" : "PRE";
		debug_printf( DEBUG_NORMAL, "Deferring %s script of node %s for %ld seconds (exit status was %d)...\n",
					prefix, script->GetNodeName(), script->_deferTime,
					script->_deferStatus );
	} else {
		script->_done = TRUE;

		// call appropriate DAG reaper
		if( ! script->_post ) {
			_dag->PreScriptReaper( script->GetNode(), status );
		} else {
			_dag->PostScriptReaper( script->GetNode(), status );
		}
	}

	// if there's another script waiting to run, run it now
	RunWaitingScripts( true );

	return 1;
}
