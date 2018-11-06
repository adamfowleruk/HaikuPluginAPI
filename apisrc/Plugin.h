#ifndef PLUGIN_H
#define PLUGIN_H

#include "PluginAPI.h"

#include <cstring>
#include <string>
#include <iostream>
#include <Message.h>
#include <Messenger.h>
#include <Handler.h>
#include <Looper.h>

/**
 * Header only library to be used by Plugin applications for convenience.
 * Handles all message packing/unpacking at the Plugin library's boundary.
 */

/** Plugin must implement this method to receive messages. All else is auto. */
    void                receive_message(const char* protocol_signature,
        BMessage* message);




/**
 * This class handles the replies and sends them to the originating thread/app.
 * This flattens a BMessage and passes it back to the originating caller.
 */ 
class ReplyHandler: public BHandler
{
public:
    ReplyHandler(void (*replyFunc)(const char*))
    : fReplyFunc(replyFunc)
    {
        // no other initialisation required
        std::cout << "ReplyHandler::ctor" << std::endl;
    };
    ~ReplyHandler()
    {
        // do NOT delete the reply function in the originating code library!!!
        std::cout << "ReplyHandler:destructor" << std::endl;
    };
    void MessageReceived(BMessage* message) {
        std::cout << "ReplyHandler::ReceiveMessage has reply to send" 
            << std::endl;
        char* flattenedMessage = new char[message->FlattenedSize()];
        message->Flatten(flattenedMessage,message->FlattenedSize());
        std::string cc(flattenedMessage);
	
        fReplyFunc(cc.c_str());
    };
private:
    void (*fReplyFunc)(const char*);
};

/**
 * This class receives replies, and manages plugin state
 */
class ReplyLooper : public BLooper
{
public:
    virtual bool            QuitRequested()
    {
        // TODO Send quit message to remote for protocol (is this needed???)
        return true;
    };
    static ReplyLooper*     instance(void (*replyFunc)(const char*))
    {
        static ReplyLooper* rl = new ReplyLooper(replyFunc);
        return rl;
    };
    ReplyHandler*           GetReplyHandler()
    {
        return fTransmitter;
    };
private:
    ReplyLooper(void (*replyFunc)(const char*)) 
        : fTransmitter(new ReplyHandler(replyFunc))
    {
        std::cout << "ReplyLooper::ctor" << std::endl;
        AddHandler(fTransmitter);
    };
    ~ReplyLooper() 
    {
        std::cout << "ReplyLooper:destructor" << std::endl;
        delete fTransmitter;
    };
    ReplyHandler*           fTransmitter;
};



/**
 * This class handles the incoming messages to the plugin library
 * It basically invokes the receiving function implemented by Plugins.
 */
class ReceivingHandler : public BHandler
{
public:
    ReceivingHandler(const char* protocol_signature)
    : fSignature(strdup(protocol_signature))
    {
        // all done in init above
        std::cout << "ReceivingHandler::ctor" << std::endl;
    };
    
    ~ReceivingHandler()
    {
        std::cout << "ReceivingHandler:destructor" << std::endl;
    }
    
    void MessageReceived(BMessage* message)
    {
        std::cout << "ReceivingHandler::MessageReceived: Invoking plugin..."
            << std::endl;
        // set up receiver for messenger instance
        receive_message(fSignature,message); // hand off to the plugin!
    };

private:
    const char* fSignature;
};

/**
 * Sends a message to the Plugin from the remote caller
 */
class ReceivingLooper : public BLooper
{
public:
    ReceivingLooper(ReceivingLooper const&) = delete;
    ReceivingLooper(ReceivingLooper &&) = delete;
    ReceivingLooper& operator=(ReceivingLooper const&) = delete;
    ReceivingLooper& operator=(ReceivingLooper &&) = delete;
    
    status_t SendMessage(BMessage *message)
    {
        std::cout << "ReceivingLooper::SendMessage" << std::endl;
        return fReceiver->SendMessage(message,fReplyTo);
    };
    static ReceivingLooper* instance(const char* protocolSignature, 
        ReplyHandler* replyTo)
    {
        static ReceivingLooper* rl = new ReceivingLooper(
            protocolSignature, replyTo);
        return rl;
    };
private:
    ReceivingLooper(const char* protocolSignature,ReplyHandler* replyTo) 
        : fIncoming(new ReceivingHandler(protocolSignature)),
          fReplyTo(replyTo)
    {
        std::cout << "ReceivingLooper::ctor" << std::endl;
        AddHandler(fIncoming);
        fReceiver = new BMessenger(fIncoming,this);
    };
    ~ReceivingLooper() 
    {
        std::cout << "ReceivingLooper:destructor" << std::endl;
        delete fReceiver;
        delete fIncoming;
    };
    ReceivingHandler*           fIncoming;
    BMessenger*                 fReceiver;
    ReplyHandler*               fReplyTo;
};


extern "C" {
    plugin_descriptor   describe_plugin();
    
    /**
     * Implementation of the receive_message function expected by the sender.
     */
    void receive_message_raw(const char* protocol_signature,
        const char* flattenedMessage, 
        void (*replyFunc)(const char *) )
    {
        // handles the message packing, unpacking, and replies. Calls above
        // unpack message
        std::cout << "Plugin.h: message_received" << std::endl;
        BMessage* message = new BMessage();
        message->Unflatten(flattenedMessage);
        // set up reply handler (Messenger)
        ReplyLooper* rl = ReplyLooper::instance(replyFunc);
        thread_id rltid = rl->Run();
        std::cout << "Plugin.h: ReplyLooper thread id: " << rltid << std::endl;
        ReceivingLooper* recl = ReceivingLooper::instance(
            protocol_signature,
            rl->GetReplyHandler()
        );
        thread_id recltid = recl->Run();
        std::cout << "Plugin.h: ReceivingLooper thread id: " << recltid 
            << std::endl;
        // now register message such that the reply goes to this looper
        recl->SendMessage(message);
        // the above will call receive_message on actual plugin
    }
}


#endif
