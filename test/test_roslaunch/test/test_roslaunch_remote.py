#!/usr/bin/env python
# Software License Agreement (BSD License)
#
# Copyright (c) 2008, Willow Garage, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials provided
#    with the distribution.
#  * Neither the name of Willow Garage, Inc. nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# Revision $Id: tcpros.py 1636 2008-07-28 20:58:29Z sfkwc $

import roslib; roslib.load_manifest('test_roslaunch')

import os, sys, unittest

import rostest
import roslaunch.core
import roslaunch.remote


## Unit tests for roslaunch.remote
class TestRoslaunchRemote(unittest.TestCase):

    
    def test_remote_node_xml(self):
        # these are fairly brittle tests, but need to make sure there aren't regressions here
        Node = roslaunch.core.Node
        n = Node('pkg1', 'type1')
        self.assertEquals('<node pkg="pkg1" type="type1" ns="/" args="" output="log" respawn="false">\n</node>', n.to_remote_xml())
        n = Node('pkg2', 'type2', namespace="/ns2/")
        self.assertEquals('<node pkg="pkg2" type="type2" ns="/ns2/" args="" output="log" respawn="false">\n</node>', n.to_remote_xml())
        # machine_name should be a noop for remote xml
        n = Node('pkg3', 'type3', namespace="/ns3/", machine_name="machine3")
        self.assertEquals('<node pkg="pkg3" type="type3" ns="/ns3/" args="" output="log" respawn="false">\n</node>', n.to_remote_xml())

        # test args
        n = Node('pkg4', 'type4', args="arg4a arg4b")
        self.assertEquals('<node pkg="pkg4" type="type4" ns="/" args="arg4a arg4b" output="log" respawn="false">\n</node>', n.to_remote_xml())
        # test respawn
        n = Node('pkg5', 'type5', respawn=True)
        self.assertEquals('<node pkg="pkg5" type="type5" ns="/" args="" output="log" respawn="true">\n</node>', n.to_remote_xml())
        n = Node('pkg6', 'type6', respawn=False)
        self.assertEquals('<node pkg="pkg6" type="type6" ns="/" args="" output="log" respawn="false">\n</node>', n.to_remote_xml())
        # test remap_args
        n = Node('pkg6', 'type6', remap_args=[('from6a', 'to6a'), ('from6b', 'to6b')])
        self.assertEquals("""<node pkg="pkg6" type="type6" ns="/" args="" output="log" respawn="false">
  <remap from="from6a" to="to6a" />
  <remap from="from6b" to="to6b" />
</node>""", n.to_remote_xml())
        # test env args
        n = Node('pkg7', 'type7', env_args=[('key7a', 'val7a'), ('key7b', 'val7b')])
        self.assertEquals("""<node pkg="pkg7" type="type7" ns="/" args="" output="log" respawn="false">
  <env name="key7a" value="val7a" />
  <env name="key7b" value="val7b" />
</node>""", n.to_remote_xml())
        # test cwd        
        n = Node('pkg8', 'type8', cwd='ros-root')
        self.assertEquals('<node pkg="pkg8" type="type8" ns="/" args="" output="log" cwd="ros-root" respawn="false">\n</node>', n.to_remote_xml())
        n = Node('pkg9', 'type9', cwd='node')
        self.assertEquals('<node pkg="pkg9" type="type9" ns="/" args="" output="log" cwd="node" respawn="false">\n</node>', n.to_remote_xml())
        # test output
        n = Node('pkg10', 'type10', output='screen')
        self.assertEquals('<node pkg="pkg10" type="type10" ns="/" args="" output="screen" respawn="false">\n</node>', n.to_remote_xml())
        n = Node('pkg11', 'type11', output='log')
        self.assertEquals('<node pkg="pkg11" type="type11" ns="/" args="" output="log" respawn="false">\n</node>', n.to_remote_xml())

        # test launch-prefix
        n = Node('pkg12', 'type12', launch_prefix='xterm -e')
        self.assertEquals('<node pkg="pkg12" type="type12" ns="/" args="" output="log" respawn="false" launch-prefix="xterm -e">\n</node>', n.to_remote_xml())        
        
        #test everything
        n = Node('pkg20', 'type20', namespace="/ns20/", machine_name="foo", remap_args=[('from20a', 'to20a'), ('from20b', 'to20b')], env_args=[('key20a', 'val20a'), ('key20b', 'val20b')], output="screen", cwd="ros-root", respawn=True, args="arg20a arg20b", launch_prefix="nice")
        self.assertEquals("""<node pkg="pkg20" type="type20" ns="/ns20/" args="arg20a arg20b" output="screen" cwd="ros-root" respawn="true" launch-prefix="nice">
  <remap from="from20a" to="to20a" />
  <remap from="from20b" to="to20b" />
  <env name="key20a" value="val20a" />
  <env name="key20b" value="val20b" />
</node>""", n.to_remote_xml())
        
    def test_remote_test_node_xml(self):
        Test = roslaunch.core.Test        
        #TODO: will certainly fail right now
        # these are fairly brittle tests, but need to make sure there aren't regressions here
        Test = roslaunch.core.Test
        n = Test('test1', 'pkg1', 'type1')
        self.assertEquals('<test test-name="test1" pkg="pkg1" type="type1" ns="/" args="" output="log">\n</test>', n.to_remote_xml())
        n = Test('test2', 'pkg2', 'type2', namespace="/ns2/")
        self.assertEquals('<test test-name="test2" pkg="pkg2" type="type2" ns="/ns2/" args="" output="log">\n</test>', n.to_remote_xml())
        # machine_name should be a noop for remote xml
        n = Test('test3', 'pkg3', 'type3', namespace="/ns3/", machine_name="machine3")
        self.assertEquals('<test test-name="test3" pkg="pkg3" type="type3" ns="/ns3/" args="" output="log">\n</test>', n.to_remote_xml())

        # test args
        n = Test('test4', 'pkg4', 'type4', args="arg4a arg4b")
        self.assertEquals('<test test-name="test4" pkg="pkg4" type="type4" ns="/" args="arg4a arg4b" output="log">\n</test>', n.to_remote_xml())
        # test remap_args
        n = Test('test6', 'pkg6', 'type6', remap_args=[('from6a', 'to6a'), ('from6b', 'to6b')])
        self.assertEquals("""<test test-name="test6" pkg="pkg6" type="type6" ns="/" args="" output="log">
  <remap from="from6a" to="to6a" />
  <remap from="from6b" to="to6b" />
</test>""", n.to_remote_xml())
        # test env args
        n = Test('test7', 'pkg7', 'type7', env_args=[('key7a', 'val7a'), ('key7b', 'val7b')])
        self.assertEquals("""<test test-name="test7" pkg="pkg7" type="type7" ns="/" args="" output="log">
  <env name="key7a" value="val7a" />
  <env name="key7b" value="val7b" />
</test>""", n.to_remote_xml())
        # test cwd        
        n = Test('test8', 'pkg8', 'type8', cwd='ros-root')
        self.assertEquals('<test test-name="test8" pkg="pkg8" type="type8" ns="/" args="" output="log" cwd="ros-root">\n</test>', n.to_remote_xml())
        n = Test('test9', 'pkg9', 'type9', cwd='node')
        self.assertEquals('<test test-name="test9" pkg="pkg9" type="type9" ns="/" args="" output="log" cwd="node">\n</test>', n.to_remote_xml())

        #test everything
        n = Test('test20', 'pkg20', 'type20', namespace="/ns20/", machine_name="foo", remap_args=[('from20a', 'to20a'), ('from20b', 'to20b')], env_args=[('key20a', 'val20a'), ('key20b', 'val20b')], cwd="ros-root", args="arg20a arg20b")
        self.assertEquals("""<test test-name="test20" pkg="pkg20" type="type20" ns="/ns20/" args="arg20a arg20b" output="log" cwd="ros-root">
  <remap from="from20a" to="to20a" />
  <remap from="from20b" to="to20b" />
  <env name="key20a" value="val20a" />
  <env name="key20b" value="val20b" />
</test>""", n.to_remote_xml())
        
if __name__ == '__main__':
    rostest.unitrun('test_roslaunch', sys.argv[0], TestRoslaunchRemote, coverage_packages=['roslaunch.remote'])
    
