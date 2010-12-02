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


#ifndef _JOBSTATE_LOG_H
#define _JOBSTATE_LOG_H

// This class handles the writing of the jobstate.log file for Pegasus
// (this file was formerly created by the Pegasus tailstatd parsing
// the dagman.out file).

// Note: the jobstate.log file is meant to be machine-parseable, so
// no changes to the format of the output should be made without
// consulting the Pegasus developers.

// Note: if a jobstate.log file is specified, failing to write to the
// file is a fatal error.

// The format of the lines in the jobstate.log file is as follows:
// Normal: <timestamp> <node name> <event name> <condor id>
//   <pegasus site> <(unused -- dash)> <sequence number>
// DAGMan start: <timestamp> INTERNAL *** DAGMAN_STARTED <condor id> ***
// DAGMan exit: <timestamp> INTERNAL *** DAGMAN_FINISHED <exit code> ***
// Recovery started: <timestamp> INTERNAL *** RECOVERY_STARTED ***
// Recovery finished: <timestamp> INTERNAL *** RECOVERY_FINISHED ***

#include <set>
#include "condor_event.h"
#include "job.h"

class JobstateLog {
public:
	/** Constructor.
	*/
	JobstateLog();

	/** Destructor.
	*/
	~JobstateLog();

	/** Set the name of the jobstate.log file.
		@param The name of the jobstate.log file to write to.
	*/
	void SetLogFile(const char *jobstateLogFile ) {
				_jobstateLogFile = strnewp( jobstateLogFile ); }

	/** Get the jobstate.log file.
		@return The jobstate.log file we're writing to.
	*/
	const char *LogFile() { return _jobstateLogFile; }

	/** Flush all pending output to the jobstate log file.
	*/
	void Flush();

	/** Set up the data structures we need to avoid re-writing previously-
		written events when we're in recovery mode.
	*/
	void InitializeRecovery();

	/** Initialize the Pegasus sequence number to the correct value
		when we're running a rescue DAG (start up one after the last
		sequence number we've already written).
	*/
	void InitializeRescue();

	/** Write the DAGMAN_STARTED "event".
		@param The Condor ID of the DAGMan job itself.
	*/
	void WriteDagmanStarted( const CondorID &DAGManJobId );

	/** Write the DAGMAN_FINISHED "event".
		@param The DAGMan exit code.
	*/
	void WriteDagmanFinished( int exitCode );

	/** Write the RECOVERY_STARTED "event".
	*/
	void WriteRecoveryStarted();

	/** Write the RECOVERY_FINISHED "event".
	*/
	void WriteRecoveryFinished();

	/** Write the RECOVERY_FAILURE "event".
	*/
	void WriteRecoveryFailure();

	/** Write an actual event.  Note that not all events will be written
		-- only the ones that Pegasus cares about.
		@param The event.
		@param The DAG node corresponding to the event.
	*/
	void WriteEvent( const ULogEvent *event, Job *node );

	/** Write the JOB_SUCCESS or JOB_FAILURE "event".
		@param The DAG node corresponding to the "event".
	*/
	void WriteJobSuccessOrFailure( Job *node );

	/** Write the [PRE|POST]_SCRIPT_STARTED "event".
		@param The DAG node corresponding to the "event".
	*/
	void WriteScriptStarted( Job *node, bool isPost );

	/** Write the [PRE|POST]_SCRIPT_SUCCESS or [PRE|POST]_SCRIPT_FAILURE
		"event".
		@param The DAG node corresponding to the "event".
	*/
	void WriteScriptSuccessOrFailure( Job *node, bool isPost );

	/** Write the SUBMIT_FAILED "event".
		@param The DAG node corresponding to the "event".
	*/
	void WriteSubmitFailure( Job *node );

private:
	/** Write an event to the jobstate.log file.
		@param The time at which this event occurred (or NULL)
		@param The DAG node corresponding to the "event".
		@param The event name.
		@param The Condor ID string (or other data).
	*/
	void Write( const time_t *eventTimeP, Job *node,
				const char *eventName, const char *condorID );

	/** Write an event to the jobstate.log file.
		@param The time at which this event occurred (or NULL)
		@param The string we want to write to the file.
	*/
	void Write( const time_t *eventTimeP, const MyString &info );

	/** Convert a CondorID to a string.
		@param The cluster part of the CondorID.
		@param The proc part of the CondorID.
		@param A MyString to receive the resulting string.
	*/
	void CondorID2Str( int cluster, int proc, MyString &idStr );

	/** Parse (partially) a line of the jobstate.log file.
		@param The line (altered by this method)
		@param The time_t reference to receive the timestamp of the event.
		@param A MyString to receive the node name.
		@param An int reference to receive the sequence number.
		@return true if parsing succeeded, false otherwise.
	*/
	static bool ParseLine( MyString &line, time_t &timestamp,
				MyString &nodeName, int &seqNum );

		// The jobstate.log file we're writing to.
	char *_jobstateLogFile;

		// File pointer to the jobstate.log file.
	FILE *_outfile;

		// When in recovery mode, this is the timestamp of the last
		// pre-recovery "real" event (used to avoid re-writing events).
	time_t _lastTimestampWritten;

		// A list of the line(s) in the jobstate.log file that have the
		// timestamp _lastTimestampWritten (used to avoid re-writing
		// events).
	std::set<MyString> _lastTimestampLines;

//TEMPTEMP -- move these out of class, into .cpp file?
		// The names of the pseudo-events we're going to write (for "real"
		// events, we use the event names defined in condor_event.h).
	static const char *JOB_SUCCESS_NAME;
	static const char *JOB_FAILURE_NAME;
	static const char *PRE_SCRIPT_STARTED_NAME;
	static const char *PRE_SCRIPT_SUCCESS_NAME;
	static const char *PRE_SCRIPT_FAILURE_NAME;
	static const char *POST_SCRIPT_STARTED_NAME;
	static const char *POST_SCRIPT_SUCCESS_NAME;
	static const char *POST_SCRIPT_FAILURE_NAME;
	static const char *INTERNAL_NAME;
	static const char *DAGMAN_STARTED_NAME;
	static const char *DAGMAN_FINISHED_NAME;
	static const char *RECOVERY_STARTED_NAME;
	static const char *RECOVERY_FINISHED_NAME;
	static const char *RECOVERY_FAILURE_NAME;
	static const char *SUBMIT_FAILURE_NAME;

		// Default Condor ID to use to check for invalid IDs.
	static const CondorID _defaultCondorID;
};

#endif	// _JOBSTATE_LOG_H
