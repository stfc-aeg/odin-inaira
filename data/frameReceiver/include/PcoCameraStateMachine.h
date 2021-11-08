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

    class PcoCameraStateUnknownCommand : public PcoCameraStateException
    {
    public:
        PcoCameraStateUnknownCommand() : PcoCameraStateException() {};
        PcoCameraStateUnknownCommand(const std::string& what) : PcoCameraStateException(what) {};
    };

    class PcoCameraStateIllegalTransition : public PcoCameraStateException
    {
    public:
        PcoCameraStateIllegalTransition() : PcoCameraStateException() {};
        PcoCameraStateIllegalTransition(const std::string& what) : PcoCameraStateException(what) {};
    };

    // Definition of state machine events
    struct EventConnect : sc::event<EventConnect> {};
    struct EventDisconnect : sc::event<EventDisconnect> {};
    struct EventArm : sc::event<EventArm> {};
    struct EventDisarm : sc::event<EventDisarm> {};
    struct EventRecordStart : sc::event<EventRecordStart> {} ;
    struct EventRecordStop : sc::event<EventRecordStop> {} ;

    // Forward declaration of states
    struct Disconnected;
    struct Connected;
    struct Armed;
    struct Recording;

    class PcoCameraLinkController;

    struct PcoCameraState : sc::state_machine<PcoCameraState, Disconnected>
    {
        PcoCameraState(PcoCameraLinkController* controller);
        void unconsumed_event(const sc::event_base& e);

        enum CommandType
        {
            CommandUnknown = -1,
            CommandConnect,
            CommandDisconnect,
            CommandArm,
            CommandDisarm,
            CommandStartRecording,
            CommandStopRecording
        };
        enum StateType
        {
            StateUnknown = -1,
            StateDisconnected,
            StateConnected,
            StateArmed,
            StateRecording
        };

        void execute_command(const char* command);
        void execute_command(std::string& command);
        void execute_command(CommandType command);

        CommandType map_command_to_type(std::string& command);
        std::string map_state_to_name(StateType state_type);

        std::string current_state_name(void);
        StateType current_state(void);

        typedef boost::bimap<std::string, CommandType> CommandTypeMap;
        typedef CommandTypeMap::value_type CommandTypeMapEntry;
        typedef boost::bimap<std::string, StateType> StateTypeMap;
        typedef StateTypeMap::value_type StateTypeMapEntry;

        void init_command_type_map(void);
        void init_state_type_map(void);

        PcoCameraLinkController* controller_;
        CommandTypeMap command_type_map_;
        StateTypeMap state_type_map_;
        boost::mutex state_transition_mutex_;
    };

    struct IStateInfo
    {
        virtual PcoCameraState::StateType state_type(void) const = 0;
    };

    struct Disconnected : IStateInfo, sc::state<Disconnected, PcoCameraState>
    {
        public:
            typedef sc::custom_reaction<EventConnect> reactions;

            Disconnected(my_context ctx) : my_base(ctx) {};

            sc::result react(const EventConnect&);

            PcoCameraState::StateType state_type(void) const
            {
                return PcoCameraState::StateType::StateDisconnected;
            }
    };

    struct Connected : IStateInfo, sc::state<Connected, PcoCameraState>
    {
        public:
            typedef mpl::list<
                sc::custom_reaction<EventDisconnect>,
                sc::custom_reaction<EventArm>
            > reactions;

            Connected(my_context ctx) : my_base(ctx) {};

            sc::result react(const EventDisconnect&);
            sc::result react(const EventArm&);

            PcoCameraState::StateType state_type(void) const
            {
                return PcoCameraState::StateType::StateConnected;
            }

    };

    struct Armed : IStateInfo, sc::state<Armed, PcoCameraState>
    {
        public:
            typedef mpl::list<
                sc::custom_reaction<EventDisarm>,
                sc::custom_reaction<EventRecordStart>
            > reactions;

            Armed(my_context ctx) : my_base(ctx) {};

            sc::result react(const EventDisarm&);
            sc::result react(const EventRecordStart&);

            PcoCameraState::StateType state_type(void) const
            {
                return PcoCameraState::StateType::StateArmed;
            }
    };

    struct Recording : IStateInfo, sc::state<Recording, PcoCameraState>
    {
        public:
            typedef sc::custom_reaction<EventRecordStop> reactions;

            Recording(my_context ctx) : my_base(ctx) {};

            sc::result react(const EventRecordStop&);

            PcoCameraState::StateType state_type(void) const
            {
                return PcoCameraState::StateType::StateRecording;
            }
    };
}
#endif // INCLUDE_PCOCAMERASTATEMACHINE_H_