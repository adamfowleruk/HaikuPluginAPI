[Back to home](../README.md)
# Proof Protocols

This document describes a set of test protocols for this repository. These mirror actual functionality required by Paladin, and so are good tests.

## Plugin types required by Paladin

Current functionality that needs refactoring to plugins:-

- Source Control (Git (Hub), Mercurial, SVN)
- Language file support (C++, yacc, lex)
- Templates (of Projects)
- Templates (of new code files, or pairs of files)
- Build output parsing (g++ output)
- Build system (g++)

Future functionality:-
- Command line output when running an app
- Log file monitoring parsing
- Tutorials that interact with Paladin

## Plugin scenarios

The above plugins can be grouped in to interaction scenarios. A test will be created for each of these as below.

## Simple calling of functionality (with or without reply)

Examples include:-
- List the functions in a source code file
- Rename a symbol/function in a source code file
- Find a symbol in a set of files
- Git add, commit
- Get template, instantiate template
- Get language file description (needs compilation? Linking? Interpreter?)

Test scenario
- Command line version of poem viewer
- Include AddPoem command (no reply) as well as get random poem
- Add get poem count, and get poem by ID to each protocol
- Built a test script that checks count increases after each add
- Run this very fast with 50 add requests, and ensure all 50 are created

## Longer running functionality

These functions may have longer running capability and so some form of progress monitoring will be required.

- Build system (Compile, link)
- Run app (and collect logs)
- Git push

Test scenario
- Count to 30 programme
- Adds one each second
- Sends progress status to calling app
- Calling app outputs progress, and maintains state throughout test

## Interactive functionality

Where the plugin may need to direct activities in the calling app. May require the library to invoke hey type commands in the calling app, as
well as implementing its own protocol to the app.

Will likely require a reply to the reply. Will mean multiple replies (Do this) from a single source message (run tutorial).

- Begin tutorial (E.g. an instruction says "Click on File -> New Project")

Test scenario
- Simple hey plugin
- Will load a hey script, and process it in the calling app
  - Means we don't have to have user interaction in the library
- Will execute File -> New project -> C++ GUI with Main Window
- Then execute Project -> Add new file -> C++ file with header
- Then open the new cpp file

## GUI plugin

Some plugins will need their own UI to interact with the user. These plugins will quit if the launching app quits. Interaction should be limited, i.e. to the functionality the calling app specifies, and so this mechanism shouldn't be used for things like full text editors.

This is the visual foundation of a generic Tutorial Plugin across all apps.

Test scenario
- Extension of the hey plugin
- Provides a list of each hey command
- Provides an 'execute' button to start it
- Provides visual feedback on which command has been executed, or is next

## Streaming with back pressure (out of scope for now)

An app that processes incoming data could invoke a plugin. That plugin could take longer to process the data than it takes to retrieve it. In this scenario, the plugin must be able to apply back pressure so the calling app "backs off" its requests.

Test scenario
- Source app adds a number to be processed once every 0.4 seconds
- Plugin processes data at a fixed speed of once per second
- Plugin reports "full speed" initially, then "back off" after messenger cache is over 50% full, then again at 75% full, and again at 90%, 95%
- Plugin reports "cease sending" at 98%
- Calling app backs off by ever increasing amount - keeps doubling delay
- Plugin then starts reporting "speed up" when below 50%, again below 40% etc
- Calling app increase by descreasing amount (log(n-speed-up-requests))
- An equilibrium of incoming per second (1) and outgoing per second (1) should be reached
- Plugin sends replies to the caller

Extended test scenario
- Caller specifies the next plugin name and protocol to pass output on to
- This second plugin takes 2.5 seconds to process each number
- Backing off and speeding up should occur throughout the stack until all stages produce data once every 2.5 seconds

Testing this particular plugin will likely require advanced logging filtering and multiple log windows in Paladin, and even performance graphing in Paladin's UI based on perf log output.
ss