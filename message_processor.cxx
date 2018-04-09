//* message processor class
/*
 * @remarks message processor for serializing messages from multi-threaded applications; uses zthreads
 */
#include <map>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <pthread.h>
#include <sys/time.h>
#include "gen/gendefs.h"
#include "gen/queue.h"
#include "gen/message_processor.h"

//* struct Message_Parms
/*
 * @brief a message, which is a command to Message_Processor::Impl::run
 * @remarks messages contain an Action, which tells Impl::run what to do, and other parameters which are indicated below,
 * next to the corresponding Action.
 */
namespace {
  //            Action              parameters
  enum Action { ACTION_KILL,       //none
                ACTION_DISPLAY_MSG //id of requesting thread, message source if, importance prefix, message string
  };
  class Message_Parms {
  public:
    Action action;
    pthread_t tid;
    int src;
    re_gen::Verbosity_Level severity;
    std::string msg;
    Message_Parms(Action  _action, pthread_t _tid, int _src, re_gen::Verbosity_Level _severity, const std::string &_msg) :
      action(_action), tid(_tid), src(_src), severity(_severity), msg(_msg) {
    }
  };
  typedef re_gen::Queue<Message_Parms> Message_Queue;
  typedef std::map<int, std::string> Message_Source_Name_Map;
  typedef std::map<int, re_gen::Verbosity_Level> Message_Source_Verbosity_Map;
  std::string msg_prefixes[] = { 
    "",        //VERBOSITY_QUIET
    "Error: ", //VERBOSITY_ERRORS
    "Info:  ", //VERBOSITY_MAJOR_STEPS,
    "Info:  ", //VERBOSITY_MINOR_STEPS,
    "Debug: "  //VERBOSITY_EVERYTHING
  };
  re_gen::Message_Processor *singleton_message_processor = 0;
#define MESSAGE_PROCESSOR_VERBOSITY re_gen::VERBOSITY_EVERYTHING
};//anonymous namespace


//* re_gen namespace
/**
 * @brief this namespace is for items that are of general interest.
 */
namespace re_gen {


//* Message_Processor::Impl class
/*
 * @brief implimentation class for effectively private function and variables of Message_Processor
 */
class Message_Processor::Impl {
public:
  bool we_are_dead;
  bool is_processing;
  int message_processor_src_id;
  Message_Queue message_queue;
  pthread_t impl_thread;
  Message_Processor *message_processor;
  Message_Source_Name_Map message_source_name_map;
  Message_Source_Verbosity_Map message_source_verbosity_map;
  Verbosity_Level overall_verbosity;

//* Message_Processor::Impl::Impl
/*
 * @brief constructor for class Message_Processor::Impl
 */
Impl(Verbosity_Level _overall_verbosity, Message_Processor *_message_processor) :
  we_are_dead(false), is_processing(false), message_processor(_message_processor), message_queue(),
  message_source_name_map(), message_source_verbosity_map(), overall_verbosity(_overall_verbosity) {
  message_processor_src_id = 0;
  message_source_name_map[message_processor_src_id] = "Message_Processor";
  message_source_verbosity_map[message_processor_src_id] = MESSAGE_PROCESSOR_VERBOSITY;
  if (pthread_create(&impl_thread, 0, run, this) != 0) {
    if (MESSAGE_PROCESSOR_VERBOSITY >= VERBOSITY_ERRORS && _overall_verbosity >= VERBOSITY_ERRORS)
      std::cerr << msg_prefixes[VERBOSITY_ERRORS] << "Impl::Impl - error in pthread_mutex_init\n" << std::flush;
    throw Gen_Err("error in pthread_mutex_init");
  }
}

~Impl() {
  Message_Parms kill_msg(ACTION_DISPLAY_MSG, pthread_self(), message_processor_src_id, VERBOSITY_MINOR_STEPS, "killing message processor");
  Message_Parms kill_act(ACTION_KILL,        pthread_self(), message_processor_src_id, VERBOSITY_QUIET, "");
  message_queue.push(kill_msg);
  message_queue.push(kill_act);
  //wait for thread to die
  for (int i = 0; !we_are_dead && i < 10; ++i) {
    //10 mS
    timeval timeout = {0, 100000};
    select(0, 0, 0, 0, &timeout);
  }
  std::string msg_to_display;
  Message_Parms message_parms(ACTION_DISPLAY_MSG, pthread_self(), message_processor_src_id, VERBOSITY_EVERYTHING, "Impl::~Impl");
  if (make_msg_to_display(message_parms, &msg_to_display))
    std::cerr << msg_to_display << std::endl;
}

//* Message_Processor::Impl::organize_importance_source_msg
/*
 * @brief organize the importance-prefix, source-prefix, and actual_msg in a message string
 * @return int - nz ==> msg should be displayed (vis-a-vis verbosity level); z ==> msg is below verbosity level - don't display
 */
bool make_msg_to_display(const Message_Parms &message_parms, std::string *const msg_to_display) {
  std::ostringstream oss("");
  std::string msg_src_str("");
  Message_Source_Name_Map::iterator pos = message_source_name_map.find(message_parms.src);
  if (pos != message_source_name_map.end()) {
    std::ostringstream src_oss;
    src_oss << pos->second;
    //if message starts with '::', then source string is a class name, and the message starts with a member function name, so no hyphin
    if (message_parms.msg.substr(0, 2) != "::")
      src_oss << " - ";
    msg_src_str.assign(src_oss.str());
  }
  //we only display the message prefix on messages that are not quiet. quiet messages are those that are displayed as part of normal
  //program output; and they are displayed verbatum.
  if (message_parms.severity == VERBOSITY_QUIET) {
    oss << message_parms.msg;
  } else {
    oss << "[" << std::setw(2) << message_parms.tid << "] " << msg_prefixes[message_parms.severity] << " " << msg_src_str << message_parms.msg;
    //no linefeed in case of a ticker message
    if (message_parms.msg.rfind(" .") != message_parms.msg.size() - 2)
      oss << std::endl;
  }
  msg_to_display->assign(oss.str());
  return((message_parms.severity <= message_source_verbosity_map[message_parms.src] && message_parms.severity <= overall_verbosity) ? true : false);
}

static void *run(void *instance) {
  return(static_cast<Message_Processor::Impl *>(instance)->loop());
}

//* Message_Processor::Impl::run
/*
 * @brief the Message_Processor worker
 */
void *loop(void) {
  int tick_count(0);
  std::string prev_msg("");
  std::string msg_to_display;
  int prev_tid(-1), prev_src(-1);
  re_gen::Verbosity_Level prev_severity(VERBOSITY_ERRORS);
  const std::string tick_string[]     = {"\b|", "\b/", "\b-", "\b\\"};
  const std::string mod_tick_string[] = {"\b!", "\bX", "\b=", "\bV" };
  do {
    try {
      is_processing = false;
      Message_Parms message_parms = message_queue.pop();
      is_processing = true;
      if (message_parms.action == ACTION_KILL) {
	we_are_dead = true;
      } else if (message_parms.action != ACTION_DISPLAY_MSG) {
	Message_Parms err_message_parms(ACTION_DISPLAY_MSG, pthread_self(), message_processor_src_id, VERBOSITY_ERRORS, "Impl::run - unknown action");
	if (make_msg_to_display(err_message_parms, &msg_to_display))
	  std::cerr << msg_to_display;
      } else if (make_msg_to_display(message_parms, &msg_to_display)) {
	if (message_parms.msg.rfind(" .") == message_parms.msg.size() - 2) {
	  //handle ticker messages
	  if (tick_count != 0) {
	    if (message_parms.src == prev_src && message_parms.tid == prev_tid) {
	      //already ticking, same source
	      if (message_parms.msg.compare(prev_msg)) {
		//new message => display new message, and restart ticker
		std::cerr << std::endl << msg_to_display << tick_string[0];
		prev_msg = message_parms.msg;
		tick_count = 1;
	      } else {
		//same message => just display a new tick mark
		std::cerr << tick_string[tick_count++ & 3];
	      }
	    } else {
	      //already ticking, new source => display modified ticker
	      std::cerr << mod_tick_string[tick_count++ & 3];
	    }
	  } else {
	    //first tick message => display message, start ticker
	    std::cerr << msg_to_display << tick_string[0];
	    prev_src = message_parms.src;
	    prev_msg = message_parms.msg;
	    prev_tid = message_parms.tid;
	    tick_count = 1;
	  }
	} else { //if (message_parms.msg.rfind(" .") == message_parms.msg.size() - 2) ...
	  //handle non-ticker (normal) messages
	  if (tick_count != 0) {
	    std::cerr << std::endl;
	    tick_count = 0;
	  } else if (prev_severity == VERBOSITY_QUIET && message_parms.severity != VERBOSITY_QUIET) {
	    //this is an error message. if the previous message was not an error message, then we need might need to add a newline
	    std::cerr << std::endl;
	  }
	  std::cerr << msg_to_display;
	  prev_src = message_parms.src;
	  prev_msg = message_parms.msg;
	  prev_tid = message_parms.tid;
	  prev_severity = message_parms.severity;
	}
      } //else if (make_msg_to_display(message_parms, &msg_to_display))
    } catch (...) {
      Message_Parms err_message_parms(ACTION_DISPLAY_MSG, pthread_self(), message_processor_src_id, VERBOSITY_ERRORS, "Impl::run - unknown exception");
      if (make_msg_to_display(err_message_parms, &msg_to_display))
	std::cerr << msg_to_display;
    }
  } while (!we_are_dead);
  is_processing = false;
  Message_Parms err_message_parms(ACTION_DISPLAY_MSG, pthread_self(), message_processor_src_id, VERBOSITY_MINOR_STEPS, "Impl::run - exit");
  if (make_msg_to_display(err_message_parms, &msg_to_display))
    std::cerr << msg_to_display << std::endl;
}
};//class Message_Processor::Impl


//* Message_Processor::Message_Processor
/*
 * @brief Message_Processor constructor
 * @note you can never access a thread object outside the scope of the variable to which it is initially assigned.
 * this is true even if you later create a pointer to the Thread that does survive the scope; and of course you cannot
 * copy the Thread object. for this reason we initially assign the Thread to the pimpl pointer. this way we can use the
 * pointer later (in the Message_Processor destructor) to cancel the thread. note that if you initially assign the
 * Thread to a pointer, then it doesn't matter wether the initial pointer, or another survives the scope.
 */
Message_Processor::Message_Processor(Verbosity_Level overall_verbosity) : pimpl(0) {
  if (singleton_message_processor != 0)
    throw re_gen::Gen_Err("Message_Processor::get_message_processor - Message_Processor singleton already initialized");
  try {
    singleton_message_processor = this;
    pimpl = new Impl(overall_verbosity, this);
    process_msg(pimpl->message_processor_src_id, VERBOSITY_EVERYTHING, "::Message_Processor - started Message_Processor");
  } catch (...) {
    if (MESSAGE_PROCESSOR_VERBOSITY >= VERBOSITY_ERRORS && overall_verbosity >= VERBOSITY_ERRORS)
      std::cerr << msg_prefixes[VERBOSITY_ERRORS] << "Message_Processor::Message_Processor - unknown exception\n" << std::flush;
    if (pimpl)
      delete pimpl;
    throw;
  }
}

//* Message_Processor::~Message_Processor
/*
 * @brief Message_Processor destructor
 * @note the kill message never gets displayed, so first we send a status message.
 * @note before killing the message processor (ie. before program termination), it's a good idea to make sure that
 * the message processor has a little time to finsish up it's queued messages... try waiting until is_idle returns true.
 */
Message_Processor::~Message_Processor() {
  if (pimpl)
    delete pimpl;
}

//* Message_Processor::set_overall_verbosity
/*
 * @brief modify the overall verbosity
 * @remarks the overall verbosity overrides each module's individual verbosity setting, but only to make the program
 * quieter. this function is necessary if you wish to instantiate the Message_Processor as a ZThread::Singleton, which
 * requires you to use a no-argument constructor.
 */
void Message_Processor::set_overall_verbosity(Verbosity_Level overall_verbosity) {
  pimpl->overall_verbosity = overall_verbosity;
}

int Message_Processor::register_msg_src(Verbosity_Level verbosity, const std::string &src_str) {
  int msg_src_id = pimpl->message_source_name_map.size();
  pimpl->message_source_name_map[msg_src_id] = src_str;
  pimpl->message_source_verbosity_map[msg_src_id] = verbosity;
  return(msg_src_id);
}

void Message_Processor::process_msg(int msg_src_id, Verbosity_Level severity, const std::string &msg_str) {
  if (severity <= pimpl->overall_verbosity && severity <= pimpl->message_source_verbosity_map[msg_src_id]) {
    Message_Parms message_parms(ACTION_DISPLAY_MSG, pthread_self(), msg_src_id, severity, msg_str);
    pimpl->message_queue.push(message_parms);
  }
}

Message_Processor *Message_Processor::get_message_processor(void) {
  if (singleton_message_processor == 0)
    throw re_gen::Gen_Err("Message_Processor::get_message_processor - Message_Processor singleton was never initialized");
  return(singleton_message_processor);
}
};//namespace re_gen

