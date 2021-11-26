/*
 * PcoCameraStateMachine.h - finite state machine implementation for the PCO camera controller
 *
 * Created on: 22 Sept 2021
 *     Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#ifndef INCLUDE_PCOCAMERASTATEMACHINE_H_
#define INCLUDE_PCOCAMERASTATEMACHINE_H_

#include <iostream>

#include <boost/statechart/event.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/custom_reaction.hpp>
#include <boost/mpl/list.hpp>
#include <boost/bimap.hpp>
#include <boost/thread/mutex.hpp>

namespace sc = boost::statechart;
namespace mpl = boost::mpl;

namespace FrameReceiver
{

    //! PcoCameraStateException - simple exception class for the state machine, implementing
    //! an information message accessible via the what() method
    class PcoCameraStateException : public std::exception
    {
    public:

        //! Create PcoCameraStateException with no message
        PcoCameraStateException(void) throw() :
            what_("")
        {};

        //! Creates PcoCameraStateException with informational message
        PcoCameraStateException(const std::string& what) throw() :
            what_(what)
        {};

        //! Returns the content of the informational message
        virtual const char* what(void) const throw()
        {
            return what_.c_str();
        };

        //! Destructor
        ~PcoCameraStateException(void) throw() {};

    private:

        // Member variables
        const std::string what_;  //!< Informational message about the exception

    }; // PcoCameraStateException

    //! PcoCameraStateUnknownCommand - PCO camera state machine exception thrown when an
    //! unrecognised state transition command name is requested
    class PcoCameraStateUnknownCommand : public PcoCameraStateException
    {
    public:
        PcoCameraStateUnknownCommand() : PcoCameraStateException() {};
        PcoCameraStateUnknownCommand(const std::string& what) : PcoCameraStateException(what) {};
    };

    //! PcoCameraStateIllegalTransition - PCO camera state machine exception thrown when a state
    //! transition is requested from a state where it is not valid
    class PcoCameraStateIllegalTransition : public PcoCameraStateException
    {
    public:
        PcoCameraStateIllegalTransition() : PcoCameraStateException() {};
        PcoCameraStateIllegalTransition(const std::string& what) : PcoCameraStateException(what) {};
    };

    //! Definition of state machine events
    struct EventConnect : sc::event<EventConnect> {};         //!< Camera connect event
    struct EventDisconnect : sc::event<EventDisconnect> {};   //!< Camera disconnect event
    struct EventArm : sc::event<EventArm> {};                 //!< Camera arm event
    struct EventDisarm : sc::event<EventDisarm> {};           //!< Camera disarm event
    struct EventRecordStart : sc::event<EventRecordStart> {}; //!< Camera recording start event
    struct EventRecordStop : sc::event<EventRecordStop> {};   //!< Camera recording stop event
    struct EventReset : sc::event<EventReset> {};             //!< Camera reset event

    //! Forward declaration of states
    struct Disconnected;
    struct Connected;
    struct Armed;
    struct Recording;
    struct Error;

    // Forward declaration of the controller class
    class PcoCameraLinkController;

    //! PcoCameraState - state machine for the PCO camera controller
    struct PcoCameraState : sc::state_machine<PcoCameraState, Disconnected>
    {
        //! State transition command type enumeration
        enum CommandType
        {
            CommandUnknown = -1,   //!< Unknown command
            CommandConnect,        //!< Connect command
            CommandDisconnect,     //!< Disconnect command
            CommandArm,            //!< Arm command
            CommandDisarm,         //!< Disarm command
            CommandStartRecording, //!< Start recording command
            CommandStopRecording,  //!< Stop recording command
            CommandReset           //!< Reset command
        };

        //! State type enumeration
        enum StateType
        {
            StateUnknown = -1,  //!< Unknown state
            StateDisconnected,  //!< Disconnected state
            StateConnected,     //!< Connected state
            StateArmed,         //!< Armed state
            StateRecording,     //!< Recording state
            StateError          //!< Error state
        };

        //! Command type map type definition
        typedef boost::bimap<std::string, CommandType> CommandTypeMap;
        //! Command type map entry type definition
        typedef CommandTypeMap::value_type CommandTypeMapEntry;
        //! State type map type definition
        typedef boost::bimap<std::string, StateType> StateTypeMap;
        //! State type map entry type definition
        typedef StateTypeMap::value_type StateTypeMapEntry;

        //! Constructor for the PcoCameraState class
        PcoCameraState(PcoCameraLinkController* controller);

        //! Executes a state transition command
        void execute_command(const char* command);
        void execute_command(std::string& command);
        void execute_command(CommandType command);

        //! Handles unconsumed, i.e. invalid, state transition events
        void unconsumed_event(const sc::event_base& event);

        //! Maps a command name string to a command type value
        CommandType map_command_to_type(std::string& command);

        //! Maps a state type to a state name
        std::string map_state_to_name(StateType state_type);

        //! Returns the current state name
        std::string current_state_name(void);

        //! Returns the current state type
        StateType current_state(void);

        //! Initialises the command type map
        void init_command_type_map(void);

        //! Initialises the state type map
        void init_state_type_map(void);

        PcoCameraLinkController* controller_; //!< Pointer to controller instance
        CommandTypeMap command_type_map_;     //!< Command type map
        StateTypeMap state_type_map_;         //!< State type map
        boost::mutex state_transition_mutex_; //!< State transition mutex
    };

    //! IStateInfo - state information interface class
    struct IStateInfo
    {
        // Returns the current state type value
        virtual PcoCameraState::StateType state_type(void) const = 0;
    };

    //! State class definitions. These inherit from the boost statechart state base class
    //! and implement the IStateInfo interface allowing the current state to be interrogated.
    //! Each state class defines one or more custom reactions to legal transition events and
    //! implements a react method for each of those transitions.

    //! Disconnected - disconnected state class
    //!
    //! The disconnected state is the default state in the camera state machine, in which the
    //! controller is not connected to the camera system.
    //!
    //! The following state transition events are supported:
    //!    connect -> connected

    struct Disconnected : IStateInfo, sc::state<Disconnected, PcoCameraState>
    {
        public:
            //! Defines reactions to legal events
            typedef sc::custom_reaction<EventConnect> reactions;

            //! Constructor - initialises state machine context
            Disconnected(my_context ctx) : my_base(ctx) {};

            //! Reacts to connect transition events
            sc::result react(const EventConnect&);

            //! Returns the current state type
            PcoCameraState::StateType state_type(void) const
            {
                return PcoCameraState::StateType::StateDisconnected;
            }
    };

    //! Connected - connected state class
    //!
    //! The connected state is occupied when the controller is connected to the camera system
    //! but the camera has not been configured and armed for image acquisition.
    //!
    //! The following state transition events are supported:
    //!
    //!    disconnect -> disconnected
    //!    arm        -> armed

    struct Connected : IStateInfo, sc::state<Connected, PcoCameraState>
    {
        public:
            //! Defines reactions to legal events
            typedef mpl::list<
                sc::custom_reaction<EventDisconnect>,
                sc::custom_reaction<EventArm>
            > reactions;

            //! Constructor - initialises state machine context
            Connected(my_context ctx) : my_base(ctx) {};

            //! Reacts to disconnect transition events
            sc::result react(const EventDisconnect&);
            //! Reacts to arm transition events
            sc::result react(const EventArm&);

            //! Returns the current state type
            PcoCameraState::StateType state_type(void) const
            {
                return PcoCameraState::StateType::StateConnected;
            }

    };

    //! Armed - armed state class
    //!
    //! The armed state is occupied when the controller has armed the camera system for image
    //! acquisition, which commits configuration settings to the camera and prepares for image
    //! acquisition.
    //!
    //! The following state transition events are supported:
    //!
    //!    disarm       -> connected
    //!    record start -> recording

    struct Armed : IStateInfo, sc::state<Armed, PcoCameraState>
    {
        public:
           //! Defines reactions to legal events
           typedef mpl::list<
                sc::custom_reaction<EventDisarm>,
                sc::custom_reaction<EventRecordStart>
            > reactions;

            //! Constructor - initialises state machine context
            Armed(my_context ctx) : my_base(ctx) {};

            //! Reacts to disarm transition events
            sc::result react(const EventDisarm&);
            //! Reacts to record start transition events
            sc::result react(const EventRecordStart&);

            //! Returns the current state type
            PcoCameraState::StateType state_type(void) const
            {
                return PcoCameraState::StateType::StateArmed;
            }
    };

    //! Recording - recording state class
    //!
    //! The recording state is occupied when the controller has started the camera in recording
    //! mode and images are being acquired.
    //!
    //! The following state transition events are supported:
    //!
    //!    record stop -> armed

    struct Recording : IStateInfo, sc::state<Recording, PcoCameraState>
    {
        public:
            //! Defines reactions to legal events
            typedef sc::custom_reaction<EventRecordStop> reactions;

            //! Constructor - initialises state machine context
            Recording(my_context ctx) : my_base(ctx) {};

            //! Reacts to record stop transition events
            sc::result react(const EventRecordStop&);

            //! Returns the current state type
            PcoCameraState::StateType state_type(void) const
            {
                return PcoCameraState::StateType::StateRecording;
            }
    };

    //! Error - error state class
    //!
    //! The error state is occupried when the controller signals that a state transition command
    //! or other camera operation has failed.
    //!
    //! The following state transition events are supported:
    //!
    //!    error -> disconnected

    struct Error : IStateInfo, sc::state<Error, PcoCameraState>
    {
        public:
            //! Defines reactions to legal events
            typedef sc::custom_reaction<EventReset> reactions;

            //! Constructor - initialises state machine context
            Error(my_context ctx) : my_base(ctx) {};

            //! Reacts to reset transition events
            sc::result react(const EventReset&);

            //! Returns the current state type
            PcoCameraState::StateType state_type(void) const
            {
                return PcoCameraState::StateType::StateError;
            }
    };
}
#endif // INCLUDE_PCOCAMERASTATEMACHINE_H_