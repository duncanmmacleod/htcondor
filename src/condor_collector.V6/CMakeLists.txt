 ###############################################################
 # 
 # Copyright 2011 Red Hat, Inc. 
 # 
 # Licensed under the Apache License, Version 2.0 (the "License"); you 
 # may not use this file except in compliance with the License.  You may 
 # obtain a copy of the License at 
 # 
 #    http://www.apache.org/licenses/LICENSE-2.0 
 # 
 # Unless required by applicable law or agreed to in writing, software 
 # distributed under the License is distributed on an "AS IS" BASIS, 
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and 
 # limitations under the License. 
 # 
 ############################################################### 


include_directories(${CMAKE_CURRENT_BINARY_DIR})

## create targets
set( collectorElements
collector_main.cpp
offline_plugin.cpp
)


set(CollectorLibSrcs
	CollectorPluginManager.cpp
	collector_stats.cpp
	collector_engine.cpp
	view_server.cpp
	collector.cpp
)

#
#condor_static_lib ( collectorlib "${CollectorLibSrcs}")

condor_daemon ( EXE condor_collector
  SOURCES "${collectorElements};${CollectorLibSrcs}"
  LIBRARIES "${CONDOR_LIBS};${CONDOR_QMF}"
  INSTALL ${C_SBIN} )
