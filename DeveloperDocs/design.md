[Back to Home](../README.md)
# Design of the Message Plugin API

The API is kept deliberately high level. It uses the C ABI for function
integration, but should still allow complex objects to be interchanged.

The mechanism used is a combination of the add-ons convention in Haiku,
plus the excellent BMessage passing, handling, and replying functionality.

The Plugin API itself is kept very high level. It basically has two methods,
allowing a plugin library to state its name and version, and the signature
version (and min/max version) of the 'protocols' it implements.

The PluginManager helper class allows each app to load its view of the plugins
available, and cache those plugins that implement particular protocols of
interest. Messages can then be passed to and from these libraries in order
to extend functionality of the calling app.

A detailed design image can be seen at [The Application Sequence Diagrm](DeveloperDocs/ApplicationPluginSequenceView.png)

## Prior Art

There are three core influences on this API: BMessage in Haiku, the hey command and scripting API, and the Tracker add-on mechanism in Haiku.

The BMessage class, and its associated classes BMessenger, BLooper, and BHandler provide a way to encapsulate and serialise/deserialise messages between apps, and handle the messages on either side of the memory boundary.

The scripting API provides a standard mid level protocol for invoking simple getter/setter functions on properties between apps. This can be used to set physical characteristics of windows, application settings, and much more.

The add-on mechanism provides a low-level (C ABI) mechanism for extending applications in Haiku, and a convention for locating plugin libraries on the file system. It does not provide a standard API set, requiring each API to define its
own convention, and each plugin to implement the convention for each and every application it wishes to be used by.

## Design Goals

To provide to Application -> Library and Library -> Library interactions the rich Messaging API of Haiku, using a standard Message Addon convention. Helper objects and code are provided to enable application developers to easily access existing functionality of plugins, and enable plugin developers to easily implement and expose their functionality to a range of applications.

On the application side, an application merely needs to create a PluginManager instance and it will discover all available plugins, and invoke their description function to determine the Protocols (in the scripting API, called Suites) that each library implements.

On the plugin side, a plugin need just implement two functions. One to describe the plugin, and another to receive a message. The message passed has built in support for replies in the usual way for BMessage instances, with no extra code required.

## Protocol Suites

In hey a set of properties to access is called a Suite. These are identified by a MIME type signature.

In a Message Plugin as described here, each Plugin implements a suite of Protocols. Each Protocol has a mime type signature. Each plugin indicates the version it implements of each protocol, and the min and max versions it can handle messages for.

Each plugin also has a name, and English summary, and a mime type signature identifying the plugin.

This richer mechanism is necessary as it is feasible that multiple libraries may implement the same protocol, whereas each app is generally unique in its scripting suites, and each app version implements only one scripting suite.

It is anticipated that an app would say "Find me all plugins that implement X protocol at Y version". This will return a list of names of the plugins implementing this protocol.

For example, a python support library could implement a python lexer, a python compiler, and a function that lists all classes/functions implemented in a python file. These are three protocols implemented by a single plugin.

Another python library may just implement a better Lexer. Both plugins can be installed on the system at the same time, and so this abstraction is necessary for them to easily co-exist.


## Implementation method

Because a BMessage's content needs passing over the memory boundary between app and library, a C ABI is best used with simple types. In this case, a BMessage has built in Flatten and Unflatten commands to convert to/from a const char* array, and so this is used for simplicity and robustness. (This mechanism is used throughout Haiku to save settings).

Because the Plugin Manager API will be compiled in to applications, it is provided as a header and c++ file. This c++ file abstracts away the underlying implementation using the PIMPL idiom.

On the plugin side, all functionality is provided in a header only API.



## Known issues

This is a PoC repository whilst I test out ideas with plugins. There are some known issues.

### Threading / multiple instances

It's legitimate, although unlikely, that multiple PluginManagers could be used by the initiating application. Whilst this is supported in the implementation of the PluginManager class, currently only one (the most recent) instance of PluginManager passes BMessage replies from the library to the calling app. 

### Libraries could call libraries

This has not been tested, but should work. One library calling multiple sub libraries may expose some undefined behaviour in the current implementation.

### Memory usage and optimisation

Some work has been done around copy/move semantics, but more can be done to make this efficient. In particular, mutliple library copies may currently be getting loaded - one per function call! This needs checking and resolving.

## Future Extensions

There are many ways this simple protocol layer could be extended. Whilst most of the work will be carried out by implementing clever protocols in plugins, it's possible the API itself may benefit from expansion in the following ways.

### Lightweight streaming data processing

Given the lightweight method of invocation it is theoretically possible to built a lightweight and high performance data processing pipeline. Akin to unix pipes, but with more modern functionality.

As an example, a single well defined streaming extension to PluginManager coule be implemented that allows for back pressure. E.g. if step 5 in the data processing flow runs slower and its message pool nearly tops out, it could pass a message to its caller to slow down, which could in turn propogate a message up the stack. This would maximise performance on complex long running streaming flows and would be purely event driven rather than using polling of threads' ready states.

### Sharing of libraries between apps

As a memory saving feature, it would be good if Haiku only loads each library once across all apps for the user. This may well be done automatically, but it should be investigated and resolved if possible.

### Message Server

In the current implementation auto discovery is supported, with no single point of failure or co-ordination between plugins and their callers. This is a modern microservice architecture idea and provides for maximum de-coupling.

I t may be desirable to control the invocation mechanisms though. For example, to blacklist particular plugins for particular protocols, or to set preferences around which plugins to take precedence for particular protocols.

This may or (more likely) may not require a message server.
