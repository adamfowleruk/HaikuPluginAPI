# An Example generic Haiku plugin API

Currently on Haiku each app has its own entry point in to each library.
Whilst fine for the majority of use cases for extending individual apps, 
when a single library can be used by multiple apps this mechanism quickly becomes unwieldy.

This repository makes an attempt at using a many:many plugin add-on API. This
builds on the existing add-ons/APP/myaddon convention to create generic
plugin libraries that can be invoked by multiple client apps.

Also currently a lot of image handling logic has to be created within
each app. This has been abstracted out in this library, making app
developers' lives much easier.

## Design of the Plugin API

The API is kept deliberately high level. It usesthe C ABI for function
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


