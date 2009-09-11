/*
 * Copyright (C) 2009, Willow Garage, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the names of Stanford University or Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ROSCPP_NODE_HANDLE_H
#define ROSCPP_NODE_HANDLE_H

#include "ros/forwards.h"
#include "ros/publisher.h"
#include "ros/subscriber.h"
#include "ros/service_server.h"
#include "ros/service_client.h"
#include "ros/subscription_message_helper.h"
#include "ros/service_message_helper.h"
#include "ros/advertise_options.h"
#include "ros/advertise_service_options.h"
#include "ros/subscribe_options.h"
#include "ros/service_client_options.h"
#include "ros/spinner.h"

#include <boost/bind.hpp>

namespace XmlRpc
{

class XmlRpcValue;

}

namespace ros
{

/** \brief Enter simple event loop
 *
 * This method enters a loop, processing callbacks.  This method should only be used
 * if the NodeHandle API is being used.
 *
 * This method is mostly useful when your node does all of its work in
 * subscription callbacks.  It will not process any callbacks that have been assigned to
 * custom queues.
 *
 */
void spin();

/** \brief Enter simple event loop
 *
 * This method enters a loop, processing callbacks.  This method should only be used
 * if the NodeHandle API is being used.
 *
 * This method is mostly useful when your node does all of its work in
 * subscription callbacks.  It will not process any callbacks that have been assigned to
 * custom queues.
 *
 * \param spinner a spinner to use to call callbacks.  Two default implementations are available,
 * SingleThreadedSpinner and MultiThreadedSpinner
 */
void spin(Spinner& spinner);
/**
 * \brief Process a single round of callbacks.
 *
 * This method is useful if you have your own loop running and would like to process
 * any callbacks that are available.  This is equivalent to calling callAvailable() on the
 * global CallbackQueue.  It will not process any callbacks that have been assigned to
 * custom queues.
 */
void spinOnce();

CallbackQueue* getGlobalCallbackQueue();

class Node;
class NodeHandleBackingCollection;

/**
 * This class is used for writing nodes.  It provides a RAII interface to ros::Node, in that
 * when the first NodeHandle is created, it instantiates a ros::Node, and when the last NodeHandle
 * goes out of scope it destroys the ros::Node.
 *
 * NodeHandle uses reference counting on the global ros::Node instance internally, and copying a NodeHandle
 * is very lightweight.
 *
 * You must call one of the ros::init functions prior to instantiating this class.
 *
 * The most widely used methods are:
 *   - Setup:
 *    - ros::init()
 *   - Publish / subscribe messaging:
 *    - advertise()
 *    - subscribe()
 *   - RPC services:
 *    - advertiseService()
 *    - serviceClient()
 *    - ros::service::call()
 *   - Parameters:
 *    - getParam()
 *    - setParam()
 */
class NodeHandle
{
public:
  /**
   * \brief Constructor
   *
   * When a NodeHandle is constructed, it checks to see if a global Node has already been instantiated.  If so, it increments the reference count
   * on the global Node.  If not, it creates the Node and sets the reference count to 1.
   *
   * \param ns Namespace for this NodeHandle.  This acts in addition to any namespace assigned to this ROS node.
   *           eg. If the node's namespace is "/a" and the namespace passed in here is "b", all topics/services/parameters
   *           will be prefixed with "/a/b/"
   * \param remappings Remappings for this NodeHandle.
   */
  NodeHandle(const std::string& ns = std::string(), const M_string& remappings = M_string());
  /**
   * \brief Copy constructor
   *
   * When a NodeHandle is copied, it inherits the namespace of the NodeHandle being copied, and increments the reference count of the global Node
   * by 1.
   */
  NodeHandle(const NodeHandle& rhs);
  /**
   * \brief Parent constructor
   *
   * This version of the constructor takes a "parent" NodeHandle, and is equivalent to:
\verbatim
NodeHandle child(parent.getNamespace() + "/" + ns);
\endverbatim
   *
   * When a NodeHandle is copied, it inherits the namespace of the NodeHandle being copied, and increments the reference count of the global Node
   * by 1.
   */
  NodeHandle(const NodeHandle& parent, const std::string& ns);
  /**
   * \brief Parent constructor
   *
   * This version of the constructor takes a "parent" NodeHandle, and is equivalent to:
\verbatim
NodeHandle child(parent.getNamespace() + "/" + ns, remappings);
\endverbatim
   *
   * This version also lets you pass in name remappings that are specific to this NodeHandle
   *
   * When a NodeHandle is copied, it inherits the namespace of the NodeHandle being copied, and increments the reference count of the global Node
   * by 1.
   */
  NodeHandle(const NodeHandle& parent, const std::string& ns, const M_string& remappings);
  /**
   * \brief Destructor
   *
   * When a NodeHandle is destroyed, it decrements the global Node's reference count by 1, and if the reference count is now 0, deletes the Node.
   */
  ~NodeHandle();

  /**
   * \brief Set the default callback queue to be used by this NodeHandle.
   *
   * Setting this will cause any callbacks from advertisements/subscriptions/services/etc. to happen through the
   * use of the specified queue.  NULL (the default) causes the global queue (serviced by ros::spin() and ros::spinOnce())
   * to be used.
   */
  void setCallbackQueue(CallbackQueueInterface* queue);

  /**
   * \brief Returns the namespace associated with this NodeHandle
   */
  const std::string& getNamespace() const { return namespace_; }

  std::string mapName(const std::string& name);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Versions of advertise()
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /**
   * \brief Advertise a topic, simple version
   *
   * This call connects to the master to publicize that the node will be
   * publishing messages on the given topic.  This method returns a Publisher that allows you to
   * publish a message on this topic.
   *
   * This version of advertise is a templated convenience function, and can be used like so
\verbatim
ros::Publisher pub = handle.advertise<std_msgs::Empty>("my_topic", 1);
\endverbatim
   *
   * \param topic Topic to advertise on
   * \param queue_size Maximum number of outgoing messages to be queued for delivery to subscribers
   * \return On success, a Publisher that, when it goes out of scope, will automatically release a reference
   * on this advertisement.  On failure, an empty Publisher.
   */
  template <class M>
  Publisher advertise(const std::string& topic, uint32_t queue_size)
  {
    AdvertiseOptions ops;
    ops.init<M>(topic, queue_size);
    return advertise(ops);
  }

  /**
   * \brief Advertise a topic, with most of the available options, including subscriber status callbacks
   *
   * This call connects to the master to publicize that the node will be
   * publishing messages on the given topic.  This method returns a Publisher that allows you to
   * publish a message on this topic.
   *
   * This version of advertise allows you to pass functions to be called when new subscribers connect and
   * disconnect.  With bare functions it can be used like so:
\verbatim
void connectCallback(const ros::PublisherPtr& pub)
{
  // Do something
}

handle.advertise<std_msgs::Empty>("my_topic", 1, connectCallback);
\endverbatim
   *
   * With class member functions it can be used with boost::bind:
\verbatim
void MyClass::connectCallback(const ros::PublisherPtr& pub)
{
  // Do something
}

MyClass my_class;
ros::Publisher pub = handle.advertise<std_msgs::Empty>("my_topic", 1, boost::bind(&MyClass::connectCallback, my_class, _1));
\endverbatim
   *
   *
   * \param topic Topic to advertise on
   * \param queue_size Maximum number of outgoing messages to be queued for delivery to subscribers
   * \param connect_cb Function to call when a subscriber connects
   * \param disconnect_cb Function to call when a subscriber disconnects
   * \param tracked_object A shared pointer to an object to track for these callbacks.  If set, the a weak_ptr will be created to this object,
   * and if the reference count goes to 0 the subscriber callbacks will not get called.
   * Note that setting this will cause a new reference to be added to the object before the
   * callback, and for it to go out of scope (and potentially be deleted) in the code path (and therefore
   * thread) that the callback is invoked from.
   * \return On success, a Publisher that, when it goes out of scope, will automatically release a reference
   * on this advertisement.  On failure, an empty Publisher which can be checked with:
\verbatim
if (handle)
{
...
}
\endverbatim
   */
  template <class M>
  Publisher advertise(const std::string& topic, uint32_t queue_size,
                            const SubscriberStatusCallback& connect_cb,
                            const SubscriberStatusCallback& disconnect_cb = SubscriberStatusCallback(),
                            const VoidPtr& tracked_object = VoidPtr())
  {
    AdvertiseOptions ops;
    ops.init<M>(topic, queue_size, connect_cb, disconnect_cb);
    ops.tracked_object = tracked_object;
    return advertise(ops);
  }

  /**
   * \brief Advertise a topic, with full range of AdvertiseOptions
   *
   * This call connects to the master to publicize that the node will be
   * publishing messages on the given topic.  This method returns a Publisher that allows you to
   * publish a message on this topic.
   *
   * This is an advanced version advertise() that exposes all options (through the AdvertiseOptions structure)
   *
   * \param ops Advertise options to use
   * \return On success, a Publisher that, when it goes out of scope, will automatically release a reference
   * on this advertisement.  On failure, an empty Publisher which can be checked with:
\verbatim
if (handle)
{
...
}
\endverbatim
   *
   */
  Publisher advertise(AdvertiseOptions& ops);


  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Versions of subscribe()
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /**
   * \brief Subscribe to a topic, version for class member function with bare pointer
   *
   * This method connects to the master to register interest in a given
   * topic.  The node will automatically be connected with publishers on
   * this topic.  On each message receipt, fp is invoked and passed a shared pointer
   * to the message received.  This message should \b not be changed in place, as it
   * is shared with any other subscriptions to this topic.
   *
   * This version of subscribe is a convenience function for using member functions, and can be used like so:
\verbatim
void Foo::callback(const std_msgs::EmptyConstPtr& message)
{
}

Foo foo_object;
ros::Subscriber sub = handle.subscribe("my_topic", 1, &Foo::callback, foo_object);
\endverbatim
   *
   * \param topic Topic to subscribe to
   * \param queue_size Number of incoming messages to queue up for
   * processing (messages in excess of this queue capacity will be
   * discarded).
   * \param fp Member function pointer to call when a message has arrived
   * \param obj Object to call fp on
   * \return On success, a Subscriber that, when all copies of it go out of scope, will unsubscribe from this topic.
   * On failure, an empty Subscriber which can be checked with:
\verbatim
if (handle)
{
...
}
\endverbatim
   */
  template<class M, class T>
  Subscriber subscribe(const std::string& topic, uint32_t queue_size, void(T::*fp)(const boost::shared_ptr<M const>&), T* obj)
  {
    SubscribeOptions ops;
    ops.init<M>(topic, queue_size, boost::bind(fp, obj, _1));
    return subscribe(ops);
  }

  /**
   * \brief Subscribe to a topic, version for class member function with shared_ptr
   *
   * This method connects to the master to register interest in a given
   * topic.  The node will automatically be connected with publishers on
   * this topic.  On each message receipt, fp is invoked and passed a shared pointer
   * to the message received.  This message should \b not be changed in place, as it
   * is shared with any other subscriptions to this topic.
   *
   * This version of subscribe is a convenience function for using member functions on a shared_ptr:
\verbatim
void Foo::callback(const std_msgs::EmptyConstPtr& message)
{
}

boost::shared_ptr<Foo> foo_object(new Foo);
ros::Subscriber sub = handle.subscribe("my_topic", 1, &Foo::callback, foo_object);
\endverbatim
   *
   * \param topic Topic to subscribe to
   * \param queue_size Number of incoming messages to queue up for
   * processing (messages in excess of this queue capacity will be
   * discarded).
   * \param fp Member function pointer to call when a message has arrived
   * \param obj Object to call fp on.  Since this is a shared pointer, the object will automatically be tracked with a weak_ptr
   * so that if it is deleted before the Subscriber goes out of scope the callback will no longer be called (and therefore will not crash).
   * \return On success, a Subscriber that, when all copies of it go out of scope, will unsubscribe from this topic.
   * On failure, an empty Subscriber which can be checked with:
\verbatim
if (handle)
{
...
}
\endverbatim
   */
  template<class M, class T>
  Subscriber subscribe(const std::string& topic, uint32_t queue_size, void(T::*fp)(const boost::shared_ptr<M const>&), const boost::shared_ptr<T>& obj)
  {
    SubscribeOptions ops;
    ops.init<M>(topic, queue_size, boost::bind(fp, obj.get(), _1));
    ops.tracked_object = obj;
    return subscribe(ops);
  }

  /**
   * \brief Subscribe to a topic, version for bare function
   *
   * This method connects to the master to register interest in a given
   * topic.  The node will automatically be connected with publishers on
   * this topic.  On each message receipt, fp is invoked and passed a shared pointer
   * to the message received.  This message should \b not be changed in place, as it
   * is shared with any other subscriptions to this topic.
   *
   * This version of subscribe is a convenience function for using bare functions, and can be used like so:
\verbatim
void callback(const std_msgs::EmptyConstPtr& message)
{
}

Foo foo_object;
ros::Subscriber sub = handle.subscribe("my_topic", 1, callback);
\endverbatim
   *
   * \param topic Topic to subscribe to
   * \param queue_size Number of incoming messages to queue up for
   * processing (messages in excess of this queue capacity will be
   * discarded).
   * \param fp Function pointer to call when a message has arrived
   * \return On success, a Subscriber that, when all copies of it go out of scope, will unsubscribe from this topic.
   * On failure, an empty Subscriber which can be checked with:
\verbatim
if (handle)
{
...
}
\endverbatim
   */
  template<class M>
  Subscriber subscribe(const std::string& topic, uint32_t queue_size, void(*fp)(const boost::shared_ptr<M const>&))
  {
    SubscribeOptions ops;
    ops.init<M>(topic, queue_size, boost::function<void(const boost::shared_ptr<M>&)>(fp));
    return subscribe(ops);
  }

  /**
   * \brief Subscribe to a topic, version for arbitrary boost::function object
   *
   * This method connects to the master to register interest in a given
   * topic.  The node will automatically be connected with publishers on
   * this topic.  On each message receipt, callback is invoked and passed a shared pointer
   * to the message received.  This message should \b not be changed in place, as it
   * is shared with any other subscriptions to this topic.
   *
   * This version of subscribe allows anything bindable to a boost::function object
   *
   * \param topic Topic to subscribe to
   * \param queue_size Number of incoming messages to queue up for
   * processing (messages in excess of this queue capacity will be
   * discarded).
   * \param callback Callback to call when a message has arrived
   * \param tracked_object A shared pointer to an object to track for these callbacks.  If set, the a weak_ptr will be created to this object,
   * and if the reference count goes to 0 the subscriber callbacks will not get called.
   * Note that setting this will cause a new reference to be added to the object before the
   * callback, and for it to go out of scope (and potentially be deleted) in the code path (and therefore
   * thread) that the callback is invoked from.
   * \return On success, a Subscriber that, when all copies of it go out of scope, will unsubscribe from this topic.
   * On failure, an empty Subscriber which can be checked with:
\verbatim
if (handle)
{
...
}
\endverbatim
   */
  template<class M>
  Subscriber subscribe(const std::string& topic, uint32_t queue_size, const boost::function<void (const boost::shared_ptr<M const>&)>& callback,
                             const VoidPtr& tracked_object = VoidPtr())
  {
    SubscribeOptions ops;
    ops.init<M>(topic, queue_size, callback);
    ops.tracked_object = tracked_object;
    return subscribe(ops);
  }

  /**
   * \brief Subscribe to a topic, version with full range of SubscribeOptions
   *
   * This method connects to the master to register interest in a given
   * topic.  The node will automatically be connected with publishers on
   * this topic.  On each message receipt, fp is invoked and passed a shared pointer
   * to the message received.  This message should \b not be changed in place, as it
   * is shared with any other subscriptions to this topic.
   *
   * This version of subscribe allows the full range of options, exposed through the SubscribeOptions class
   *
   * \param ops Subscribe options
   * \return On success, a Subscriber that, when all copies of it go out of scope, will unsubscribe from this topic.
   * On failure, an empty Subscriber which can be checked with:
\verbatim
if (handle)
{
...
}
\endverbatim
   */
  Subscriber subscribe(SubscribeOptions& ops);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Versions of advertiseService()
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /**
   * \brief Advertise a service, version for class member function with bare pointer
   *
   * This call connects to the master to publicize that the node will be
   * offering an RPC service with the given name.
   *
   * This is a convenience function for using member functions, and can be used like so:
\verbatim
bool Foo::callback(std_srvs::Empty& request, std_srvs::Empty& response)
{
  return true;
}

Foo foo_object;
ros::ServiceServer service = handle.advertiseService("my_service", &Foo::callback, foo_object);
\endverbatim
   *
   * \param service Service name to advertise on
   * \param srv_func Member function pointer to call when a message has arrived
   * \param obj Object to call srv_func on
   * \return On success, a ServiceServer that, when all copies of it go out of scope, will unadvertise this service.
   * On failure, an empty ServiceServer which can be checked with:
\verbatim
if (handle)
{
...
}
\endverbatim
   */
  template<class T, class MReq, class MRes>
  ServiceServer advertiseService(const std::string& service, bool(T::*srv_func)(MReq &, MRes &), T *obj)
  {
    AdvertiseServiceOptions ops;
    ops.init<MReq, MRes>(service, boost::bind(srv_func, obj, _1, _2));
    return advertiseService(ops);
  }

  /**
   * \brief Advertise a service, version for class member function with shared_ptr
   *
   * This call connects to the master to publicize that the node will be
   * offering an RPC service with the given name.
   *
   * This is a convenience function for using member functions on shared pointers, and can be used like so:
\verbatim
bool Foo::callback(std_srvs::Empty& request, std_srvs::Empty& response)
{
  return true;
}

boost::shared_ptr<Foo> foo_object(new Foo);
ros::ServiceServer service = handle.advertiseService("my_service", &Foo::callback, foo_object);
\endverbatim
   *
   * \param service Service name to advertise on
   * \param srv_func Member function pointer to call when a message has arrived
   * \param obj Object to call srv_func on.  Since this is a shared_ptr, it will automatically be tracked with a weak_ptr,
   * and if the object is deleted the service callback will stop being called (and therefore will not crash).
   * \return On success, a ServiceServer that, when all copies of it go out of scope, will unadvertise this service.
   * On failure, an empty ServiceServer which can be checked with:
\verbatim
if (handle)
{
...
}
\endverbatim
   */
  template<class T, class MReq, class MRes>
  ServiceServer advertiseService(const std::string& service, bool(T::*srv_func)(MReq &, MRes &), const boost::shared_ptr<T>& obj)
  {
    AdvertiseServiceOptions ops;
    ops.init<MReq, MRes>(service, boost::bind(srv_func, obj.get(), _1, _2));
    ops.tracked_object = obj;
    return advertiseService(ops);
  }

  /**
   * \brief Advertise a service, version for bare function
   *
   * This call connects to the master to publicize that the node will be
   * offering an RPC service with the given name.
   *
   * This is a convenience function for using bare functions, and can be used like so:
\verbatim
bool callback(std_srvs::Empty& request, std_srvs::Empty& response)
{
  return true;
}

ros::ServiceServer service = handle.advertiseService("my_service", callback);
\endverbatim
   *
   * \param service Service name to advertise on
   * \param srv_func function pointer to call when a message has arrived
   * \return On success, a ServiceServer that, when all copies of it go out of scope, will unadvertise this service.
   * On failure, an empty ServiceServer which can be checked with:
\verbatim
if (handle)
{
...
}
\endverbatim
   */
  template<class MReq, class MRes>
  ServiceServer advertiseService(const std::string& service, bool(*srv_func)(MReq&, MRes&))
  {
    AdvertiseServiceOptions ops;
    ops.init<MReq, MRes>(service, boost::function<bool(MReq&, MRes&)>(srv_func));
    return advertiseService(ops);
  }

  /**
   * \brief Advertise a service, version for arbitrary boost::function object
   *
   * This call connects to the master to publicize that the node will be
   * offering an RPC service with the given name.
   *
   * This version of advertiseService allows non-class functions, as well as functor objects and boost::bind (along with anything
   * else boost::function supports).
   *
   * \param service Service name to advertise on
   * \param callback Callback to call when the service is called
   * \param tracked_object A shared pointer to an object to track for these callbacks.  If set, the a weak_ptr will be created to this object,
   * and if the reference count goes to 0 the subscriber callbacks will not get called.
   * Note that setting this will cause a new reference to be added to the object before the
   * callback, and for it to go out of scope (and potentially be deleted) in the code path (and therefore
   * thread) that the callback is invoked from.
   * \return On success, a ServiceServer that, when all copies of it go out of scope, will unadvertise this service.
   * On failure, an empty ServiceServer which can be checked with:
\verbatim
if (handle)
{
...
}
\endverbatim
   */
  template<class MReq, class MRes>
  ServiceServer advertiseService(const std::string& service, const boost::function<bool(MReq&, MRes&)>& callback, const VoidPtr& tracked_object = VoidPtr())
  {
    AdvertiseServiceOptions ops;
    ops.init<MReq, MRes>(service, callback);
    ops.tracked_object = tracked_object;
    return advertiseService(ops);
  }

  /**
   * \brief Advertise a service, with full range of AdvertiseServiceOptions
   *
   * This call connects to the master to publicize that the node will be
   * offering an RPC service with the given name.
   *
   * This version of advertiseService allows the full set of options, exposed through the AdvertiseServiceOptions class
   *
   * \param ops Advertise options
   * \return On success, a ServiceServer that, when all copies of it go out of scope, will unadvertise this service.
   * On failure, an empty ServiceServer which can be checked with:
\verbatim
if (handle)
{
...
}
\endverbatim
   */
  ServiceServer advertiseService(AdvertiseServiceOptions& ops);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Versions of serviceClient()
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////

  /** @brief Create a client for a service, version templated on two message types
   *
   * When the last handle reference of a persistent connection is cleared, the connection will automatically close.
   *
   * @param service_name The name of the service to connect to
   * @param persistent Whether this connection should persist.  Persistent services keep the connection to the remote host active
   *        so that subsequent calls will happen faster.  In general persistent services are discouraged, as they are not as
   *        robust to node failure as non-persistent services.
   * @param header_values Key/value pairs you'd like to send along in the connection handshake
   */
  template<class MReq, class MRes>
  ServiceClient serviceClient(const std::string& service_name, bool persistent = false, const M_string& header_values = M_string())
  {
    ServiceClientOptions ops;
    ops.init<MReq, MRes>(service_name, persistent, header_values);
    return serviceClient(ops);
  }

  /** @brief Create a client for a service, version templated on service type
   *
   * When the last handle reference of a persistent connection is cleared, the connection will automatically close.
   *
   * @param service_name The name of the service to connect to
   * @param persistent Whether this connection should persist.  Persistent services keep the connection to the remote host active
   *        so that subsequent calls will happen faster.  In general persistent services are discouraged, as they are not as
   *        robust to node failure as non-persistent services.
   * @param header_values Key/value pairs you'd like to send along in the connection handshake
   */
  template<class Service>
  ServiceClient serviceClient(const std::string& service_name, bool persistent = false, const M_string& header_values = M_string())
  {
    ServiceClientOptions ops;
    ops.init<Service>(service_name, persistent, header_values);
    return serviceClient(ops);
  }

  /** @brief Create a client for a service, version with full range of ServiceClientOptions
   *
   * When the last handle reference of a persistent connection is cleared, the connection will automatically close.
   *
   * @param ops The options for this service client
   */
  ServiceClient serviceClient(ServiceClientOptions& ops);

  /** \brief Set an arbitrary XML/RPC value on the parameter server.
   *
   * \param key The key to be used in the parameter server's dictionary
   * \param v The value to be inserted.
   */
  void setParam(const std::string& key, const XmlRpc::XmlRpcValue& v);
  /** \brief Set a string value on the parameter server.
   *
   * \param key The key to be used in the parameter server's dictionary
   * \param s The value to be inserted.
   */
  void setParam(const std::string& key, const std::string& s);
  /** \brief Set a string value on the parameter server.
   *
   * \param key The key to be used in the parameter server's dictionary
   * \param s The value to be inserted.
   */
  void setParam(const std::string& key, const char* s);
  /** \brief Set a double value on the parameter server.
   *
   * \param key The key to be used in the parameter server's dictionary
   * \param d The value to be inserted.
   */
  void setParam(const std::string& key, double d);
  /** \brief Set a integer value on the parameter server.
   *
   * \param key The key to be used in the parameter server's dictionary
   * \param i The value to be inserted.
   */
  void setParam(const std::string& key, int i);
  /** \brief Set a integer value on the parameter server.
   *
   * \param key The key to be used in the parameter server's dictionary
   * \param b The value to be inserted.
   */
  void setParam(const std::string& key, bool b);

  /** \brief Get a string value from the parameter server.
   *
   * \param key The key to be used in the parameter server's dictionary
   * \param[out] s Storage for the retrieved value.
   * \param use_cache Determines whether or not we will cache and subscribe
   * to updates for this parameter.  If use_cache is true and we don't have
   * this parameter cached, we will subscribe to updates for this parameter
   * from the parameter server and cache the value for fast access.
   * If use_cache is false, we always hit the parameter server to request
   * the value.
   *
   * \return true if the parameter value was retrieved, false otherwise
   */
  bool getParam(const std::string& key, std::string& s, bool use_cache = false);
  /** \brief Get a double value from the parameter server.
   *
   * \param key The key to be used in the parameter server's dictionary
   * \param[out] d Storage for the retrieved value.
   * \param use_cache Determines whether or not we will cache and subscribe
   * to updates for this parameter.  If use_cache is true and we don't have
   * this parameter cached, we will subscribe to updates for this parameter
   * from the parameter server and cache the value for fast access.
   * If use_cache is false, we always hit the parameter server to request
   * the value.
   *
   * \return true if the parameter value was retrieved, false otherwise
   */
  bool getParam(const std::string& key, double& d, bool use_cache = false);
  /** \brief Get a integer value from the parameter server.
   *
   * \param key The key to be used in the parameter server's dictionary
   * \param[out] i Storage for the retrieved value.
   * \param use_cache Determines whether or not we will cache and subscribe
   * to updates for this parameter.  If use_cache is true and we don't have
   * this parameter cached, we will subscribe to updates for this parameter
   * from the parameter server and cache the value for fast access.
   * If use_cache is false, we always hit the parameter server to request
   * the value.
   *
   * \return true if the parameter value was retrieved, false otherwise
   */
  bool getParam(const std::string& key, int& i, bool use_cache = false);
  /** \brief Get a boolean value from the parameter server.
   *
   * \param key The key to be used in the parameter server's dictionary
   * \param[out] b Storage for the retrieved value.
   * \param use_cache Determines whether or not we will cache and subscribe
   * to updates for this parameter.  If use_cache is true and we don't have
   * this parameter cached, we will subscribe to updates for this parameter
   * from the parameter server and cache the value for fast access.
   * If use_cache is false, we always hit the parameter server to request
   * the value.
   *
   * \return true if the parameter value was retrieved, false otherwise
   */
  bool getParam(const std::string& key, bool& b, bool use_cache = false);
  /** \brief Get an arbitrary XML/RPC value from the parameter server.
   *
   * \param key The key to be used in the parameter server's dictionary
   * \param[out] v Storage for the retrieved value.
   * \param use_cache Determines whether or not we will cache and subscribe
   * to updates for this parameter.  If use_cache is true and we don't have
   * this parameter cached, we will subscribe to updates for this parameter
   * from the parameter server and cache the value for fast access.
   * If use_cache is false, we always hit the parameter server to request
   * the value.
   *
   * \return true if the parameter value was retrieved, false otherwise
   */
  bool getParam(const std::string& key, XmlRpc::XmlRpcValue& v, bool use_cache = false);

  /** \brief Check whether a parameter exists on the parameter server.
   *
   * \param key The key to check.
   *
   * \return true if the parameter exists, false otherwise
   */
  bool hasParam(const std::string& key);
  /** \brief Delete a parameter from the parameter server.
   *
   * \param key The key to delete.
   *
   * \return true if the deletion succeeded, false otherwise.
   */
  bool deleteParam(const std::string& key);

  /** \brief Assign value from parameter server, with default.
   *
   * This method tries to retrieve the indicated parameter value from the
   * parameter server, storing the result in param_val.  If the value
   * cannot be retrieved from the server, default_val is used instead.
   *
   * \param param_name The key to be searched on the parameter server.
   * \param[out] param_val Storage for the retrieved value.
   * \param default_val Value to use if the server doesn't contain this
   * parameter.
   */
  template<typename T>
  void param(const std::string& param_name, T& param_val, const T& default_val)
  {
    if (hasParam(param_name))
    {
      if (getParam(param_name, param_val))
      {
        return;
      }
    }

    param_val = default_val;
  }

  /**
   * \brief Shutdown every handle created through this NodeHandle.
   *
   * This method will unadvertise every topic and service advertisement,
   * and unsubscribe every subscription created through this NodeHandle.
   */
  void shutdown();

  /** \brief Check whether it's time to exit.
   *
   * This method checks the value of Node::ok(), to see whether it's yet time
   * to exit.  ok() is false once shutdown() has been called
   *
   * \return true if we're still OK, false if it's time to exit
   */
  bool ok() const;

  /**
   * \brief Set the max time this node should spend looping trying to connect to the master
   * @param milliseconds the timeout, in milliseconds.  A value of -1 means infinite
   */
  void setMasterRetryTimeout(int32_t milliseconds);

  /** \brief Get the list of topics advertised by this node
   *
   * \param[out] topics The advertised topics
   */
  void getAdvertisedTopics(V_string& topics) const;

  /** \brief Get the list of topics subscribed to by this node
   *
   * \param[out] The subscribed topics
   */
  void getSubscribedTopics(V_string& topics) const;

  /**
   * \brief Returns a pointer to the node being used by this NodeHandle.
   */
  Node* getNode() const { return node_; }

  const std::string& getName() const;

  /**
   * \brief Get the args we parsed out of argv in ros::init()
   */
  static const V_string& getParsedArgs();

  /** \brief Get the hostname where the master runs.
   *
   * \return The master's hostname, as a string
   */
  const std::string &getMasterHost() const;
  /** \brief Get the port where the master runs.
   *
   * \return The master's port.
   */
  int getMasterPort() const;
  /**
   * \brief Get the xmlrpc URI of this node
   */
  const std::string& getXMLRPCURI() const;

  /** \brief Check whether the master is up
   *
   * This method tries to contact the master.  The intended usage is to check
   * whether the master is up before trying to make other requests
   * (subscriptions, advertisements, etc.).
   *
   * \return true if the master is available, false otherwise.
   */
  bool checkMaster();

  /** \brief Get the list of topics that are being published by all nodes.
   *
   * This method communicates with the master to retrieve the list of all
   * currently advertised topics.
   *
   * \param topics A place to store the resulting list.  Each item in the
   * list is a pair <topic, type>.  The type is represented in the same
   * way it is used inside a message definition (eg. std_msgs/String)
   *
   * \return true on success, false otherwise (topics not filled in)
   */
  bool getPublishedTopics(VP_string& topics);

private:
  void construct();
  void destruct();

  void initRemappings(const M_string& remappings);

  std::string namespace_;
  ros::Node* node_;
  M_string remappings_;

  CallbackQueueInterface* callback_queue_;

  NodeHandleBackingCollection* collection_;
};

}

#endif // ROSCPP_NODE_HANDLE_H
