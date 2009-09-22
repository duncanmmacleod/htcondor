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

#ifndef __SHARED_PORT_LISTENER_H__
#define __SHARED_PORT_LISTENER_H__

#include "condor_daemon_core.h"
#include "reli_sock.h"
#include "selector.h"

// SharedPortEndpoint receives connections forwarded from SharedPortServer.
// This enables Condor daemons to share a single network port.

class SharedPortEndpoint: Service {
 public:

	SharedPortEndpoint();
	~SharedPortEndpoint();

	void InitAndReconfig();

		// create our named socket for receiving connections
		// from the shared port server
	bool CreateListener();

		// call CreateListener() and register with daemonCore
	bool StartListener();

	void StopListener();

		// returns a contact string suitable for connecting to the
		// SharedPortServer that will forward the connection to us.
		// May return NULL if remote address cannot be determined.
	char const *GetMyRemoteAddress();

		// Force an immediate reload of the shared port server's
		// address.
	void ReloadSharedPortServerAddr();

		// returns a contact string suitable for direct connection
		// to this daemon from the same machine without going through
		// SharedPortServer
	char const *GetMyLocalAddress();

		// returns local id (name of shared socket)
	char const *GetSharedPortID();

		// Return the path to the named socket.  It is nice for the
		// parent to clean this file up in case the child does not.
	char const *GetSocketFileName();

		// seconds between touching the named socket
	static int TouchSocketInterval();

		// Appends string to buffer and sets file descriptor that needs
		// to be inherited so that this object can be reconstructed
		// in a child process.
	void serialize(MyString &inherit_buf,int &inherit_fd);

		// Restore state of object stored with serialize().
		// Returns pointer to anything trailing in inherit_buf.
	char *deserialize(char *inherit_buf);

		// Do not remove named socket when we stop listening.
		// Used in parent process when passing this object to a child.
	void Detach();

		// Make the named socket owned such that it can be removed
		// by a process with the specified priv state.
	bool ChownSocket(priv_state priv);

		// Remove named socket
	static bool RemoveSocket( char const *fname );

		// Used by CCB client to manage asynchronous events from the
		// shared port listener and the CCB server.
	void AddListenerToSelector(Selector &selector);
	void RemoveListenerFromSelector(Selector &selector);
	bool CheckListenerReady(Selector &selector);

		// returns true if this process should use a shared port
	static bool UseSharedPort(MyString *why_not=NULL,bool already_open=false);

	void DoListenerAccept(ReliSock *return_remote_sock);

 private:
	bool m_listening;
	bool m_registered_listener;
	MyString m_socket_dir;// dirname of socket
	MyString m_full_name; // full path of socket
	MyString m_local_id;  // basename of socket
	MyString m_remote_addr;  // SharedPortServer addr with our local_id inserted
	MyString m_local_addr;
	int m_retry_remote_addr_timer;
	ReliSock m_listener_sock; // named socket to receive forwarded connections
	int m_socket_check_timer;

	int HandleListenerAccept( Stream * stream );

	void ReceiveSocket( ReliSock *local_sock, ReliSock *return_remote_sock );

	bool InitRemoteAddress();
	int RetryInitRemoteAddress();
		// the return value is junk to match TimerHandlercpp
	int SocketCheck();

	bool MakeDaemonSocketDir();
};

#endif
