#pragma once

#include <Stream.h>

// Adapted from the bundled SimpleFOC 2.2 Commander.
// Generic command support only; no motor, PID, or filter dependencies.
namespace ffb {

// Commander verbose display to the user type
enum VerboseMode {
    nothing = 0,   // display nothing - good for monitoring
    on_request,    // display command results without descriptive text
    user_friendly  // display textual messages to the user
};

// callback function pointer definition
using CommandCallback = void (*)(char *);  //!< command callback function pointer

/**
 * Commander class implementing string communication based on IDvalue
 * (ex. AB5.321 - command ID `A`, sub-command ID `B`, value `5.321`).
 *
 *  - This class can read and write an Arduino Stream instance, or parse
 *    strings received by the user outside this library.
 *  - Commander provides a command > callback interface that enables the user
 *    to attach a callback function to a command ID - see add().
 *  - The scalar() helper implements float variable queries and updates.
 */
class Commander {
   public:
    static constexpr int MaxCommands = 20;       //!< maximum number of subscribed callbacks
    static constexpr int MaxCommandLength = 20;  //!< maximum collected characters, including the line sentinel

    /**
     * Constructor receiving a serial interface used to output command results.
     * run() also uses this interface to read user commands.
     *
     * @param serial - serial Stream instance initialized by the caller
     * @param eol - end of line sentinel character
     * @param echo - echo typed characters for command line feedback
     */
    Commander(Stream &serial, char eol = '\n', bool echo = false);
    /**
     * Constructor without a bound serial interface.
     * Commands can be supplied through run(Stream &, char) or run(char *).
     *
     * @param eol - end of line sentinel character
     * @param echo - echo typed characters for command line feedback
     */
    Commander(char eol = '\n', bool echo = false);

    /**
     * Read the bound serial port and fire subscribed callbacks when a complete
     * command is received. Partial input is retained between calls.
     *
     *  - Default commands:
     *    '@' - Verbose mode
     *    '#' - Number of decimal places
     *    '?' - Scan command - display IDs and labels of subscribed callbacks
     */
    void run();
    /**
     * Read a temporary serial interface and fire subscribed callbacks.
     * Default commands are '@', '#' and '?', as in run().
     * The previous serial interface and sentinel are restored after this call.
     *
     * @param serial - temporary Stream used for command input and output
     * @param eol - temporary end of line sentinel character
     */
    void run(Stream &serial, char eol = '\n');
    /**
     * Parse a supplied command and fire the callback registered for its first
     * character. The remaining characters are passed to the callback.
     * Default commands are '@', '#' and '?', as in run().
     *
     * @param user_input - NUL-terminated command, including the line sentinel
     *                    when querying a scalar or Commander setting
     */
    void run(char *user_input);

    /**
     * Add a callback to Commander with a command ID.
     * The first matching subscribed ID is used; full tables and null callbacks
     * are ignored. The callback and label must remain valid while registered.
     * Callback arguments are valid for the duration of the callback.
     *
     * @param id - command character
     * @param onCommand - callback function void function(char *)
     * @param label - description displayed by the scan command
     */
    void add(char id, CommandCallback onCommand, const char *label = nullptr);

    /**
     * Float variable scalar command interface.
     *  - Query by sending only the line sentinel (normally '\n').
     *  - Set by sending a value (ex. 0.01f to set *value = 0.01).
     *  - Conversion uses atof(), as in the original Commander.
     * The current value is printed after a query or update.
     *
     * @param value - float variable to query or update
     * @param user_cmd - callback argument, including the line sentinel
     */
    void scalar(float *value, char *user_cmd);

    // printing variables
    VerboseMode verbose = VerboseMode::user_friendly;  //!< flag controlling command results and explanatory text
    uint8_t decimal_places = 3;                        //!< number of decimal places used when displaying floats

    // serial interface configuration
    Stream *com_port = nullptr;  //!< serial terminal instance, if provided
    char eol = '\n';             //!< end of line sentinel character
    bool echo = false;           //!< echo typed characters for command line feedback

   private:
    // Subscribed command callback variables
    CommandCallback call_list[MaxCommands]{};  //!< array of command callback pointers
    char call_ids[MaxCommands]{};              //!< subscribed command IDs
    const char *call_label[MaxCommands]{};     //!< subscribed command descriptions
    int call_count = 0;                        //!< number of subscribed callbacks

    // helping variables for serial communication reading
    // One extra byte keeps the input NUL-terminated for conversion functions.
    char received_chars[MaxCommandLength + 1]{};  //!< received message awaiting the line sentinel
    int rec_cnt = 0;                              //!< number of received characters

    // serial printing functions
    /**
     * Print explanatory text only in user_friendly verbose mode.
     *
     * @param message - text to print
     */
    void printVerbose(const char *message);
    /**
     * Print explanatory text only in user_friendly verbose mode.
     * Handles strings defined by the F macro.
     *
     * @param message - Flash string to print
     */
    void printVerbose(const __FlashStringHelper *message);
    /**
     * Print values to the serial interface unless verbose mode is nothing.
     * Float output uses decimal_places; println() appends a newline.
     */
    void print(float number);
    void print(int number);
    void print(const char *message);
    void print(const __FlashStringHelper *message);
    void print(char message);
    void println(float number);
    void println(int number);
    void println(const char *message);
    void println(const __FlashStringHelper *message);
    void println(char message);
    void printError();         //!< print the command error response
    bool isSentinel(char ch);  //!< test the line sentinel and warn about carriage returns
};

}  // namespace ffb
