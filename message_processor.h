//* message_processor.h - header file for the Message_Processor class
/*
 * @brief this header file defines the Message_Processor class and related constants.
 */
#ifndef __IF_MESSAGE_PROCESSOR__
#define __IF_MESSAGE_PROCESSOR__
#include <string>
#include <unistd.h>
#include <gen/gendefs.h>

//* re_gen namespace
/**
 * @brief this namespace is for items that are of general interest.
 */
namespace re_gen {

//* Message_Processor class
/*
 * @brief the Message_Processor encapulates a thread whose sole function is to display Messages
 *
 * @remarks one Message_Processor (singleton) should be constructed for the entire program; all threads that want to issue messages
 * should do so via the process_message function.
 *
 * @note pass the name of the module or thread group to the register_msg_src function. from then on messages from that messgae source will be prefixed
 * with the name and a hyphin, "name - ". if however the message is passed into the process_msg function begins with "::", then the the prefix is just
 * the name. this is to accomodate the usage in which the name is actually the name of a class, and the message starts with the name of a member function.
 *
 * @note if you use a no-argument constructor, then the default overall_verbosity, if it is set fairly quietly, may be such that debug messages relating
 * to construction of the message processor are not displayed; and even if you subsequently set the verbosity to be noisy-er, you will miss those initial
 * messages.... just so you know...
 *
 * @note one source cannot, with its tick message, pre-empt another source's tick messages; but the same source can preempt its own tick messages - unless
 * the thread_id's of the two tick messages are different.
 */
class Message_Processor {
 protected:
  class Impl;
  Impl *pimpl;

 public:
  Message_Processor(re_gen::Verbosity_Level overall_verbosity = re_gen::VERBOSITY_MINOR_STEPS);
  ~Message_Processor();
  int register_msg_src(Verbosity_Level verbosity, const std::string &src_str);
  void process_msg(int msg_src_id, Verbosity_Level importance, const std::string &msg_str);
  void set_overall_verbosity(Verbosity_Level overall_verbosity);
  static Message_Processor *get_message_processor(void);
 private:
  Message_Processor(const Message_Processor &);
  Message_Processor& operator=(const Message_Processor &);
};//class Message_Processor
};//namespace re_gen
#endif //__IF_MESSAGE_PROCESSOR__
