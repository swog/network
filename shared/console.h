#pragma once

typedef void (*console_commandfn)(class command_args& args);

#define CONSOLE_COMMAND(name, help, flags) \
	static void name##_cmd_f(command_args& args); \
	static console_command name##_cmd(#name, help, flags, name##_cmd_f); \
	static void name##_cmd_f(command_args& args)

#define CONSOLE_VAR_CHANGE(name, help, flags, default_value) \
	static void name##_change_f(console_var& var, const char* str, int num, float fl); \
	static console_var name(#name, help, flags, default_value, name##_change_f); \
	static void name##_change_f(console_var& var, const char* str, int num, float fl)

#define CONSOLE_VAR(name, help, flags, default_value) \
	static console_var name(#name, help, flags, default_value)

#ifdef _SERVER
// Set log client thru svc_print
void sv_con_log(std::shared_ptr<client> cl);
void sv_con_unlog();

void sv_con_setrcon(bool rcon);
#endif

// This method provides logging functionality for echoing to client
void con_printf(const char* format, ...);

enum {
	fcommand_hidevalue = 1,	// Hide the value of a console variable
};

// More accurately a command execution packet
class command_args {
public:
	command_args(const char* str);

	const std::string& operator[](size_t i) const {
		return argv[i];
	}

	std::string& operator[](size_t i) {
		return argv[i];
	}

	const size_t size() const {
		return argv.size();
	}

	// The command to execute
	// If the command is not found, the command is not queued to the main thread
	class console_command* cmd;
	// Argument string following the space after the command name
	std::string argstr;
	// Tokenized arguments
	// argv[0] = command name
	std::vector<std::string> argv;
};

//=============================================================================
// 
// Console command section
// 
//=============================================================================
class console_command {
public:
	friend class command_args;
	friend class console;

	friend static void list_cmd_f(command_args& args);

	friend class console_var;

	console_command(const char* name, const char* help, size_t flags, console_commandfn fn) {
		init(name, help, flags, fn);
	}

	virtual bool is_var() const {
		return false;
	}

	const char* name() const {
		return _name;
	}

	const char* help() const {
		return _help;
	}

	const size_t& flags() const {
		return _flags;
	}

	size_t& flags() {
		return _flags;
	}

	const console_commandfn& fn() const {
		return _fn;
	}

	console_commandfn& fn() {
		return _fn;
	}

	void execute(command_args& args) {
		if (_fn)
			_fn(args);
	}

private:
	// Required because we inherit in console_var
	// Private so that devs can't define without name, etc
	console_command();

	void init(const char* name, const char* help, size_t flags, console_commandfn fn);

	const char* _name;
	const char* _help;
	size_t _flags;
	console_commandfn _fn;
	console_command* _next;
};

//=============================================================================
// 
// Console variable section
// 
//=============================================================================

// Console variable change function
// `var` contains the old values
typedef void (*console_var_changefn)(class console_var& var, const char* str,
	int num, float fl);

class console_var : public console_command {
public:
	console_var(const char* name, const char* help, size_t flags, const char* val, console_var_changefn changefn = NULL);

	console_var(const char* name, const char* help, size_t flags, int num, console_var_changefn changefn = NULL);

	console_var(const char* name, const char* help, size_t flags, float fl, console_var_changefn changefn = NULL);

	virtual bool is_var() const {
		return true;
	}

	template<class T>
	void set_value(const T& val) {
		set_value(std::to_string(val));
	}

	void set_value(const char* str) {
		set_value(std::string(str));
	}

	void set_value(const std::string& str);

	const std::string& get_string() const {
		return _str;
	}

	const char* get_cstr() const {
		return _str.c_str();
	}

	const int& get_num() const {
		return _num;
	}

	const float& get_float() const {
		return _fl;
	}

private:
	std::string _str;
	int _num;
	float _fl;
	console_var_changefn _changefn;
	static console_commandfn _varfn;
};

//=============================================================================
// 
// Console instance section
// 
//=============================================================================
class console& get_console();

class console {
public:
	friend class console_command;
	friend class console_var;
	friend class console;
	friend class command_args;

	// Default command to list all commands.
	// Visual studio cant find its definition because of the CONSOLE_COMMAND macro, search for `CONSOLE_COMMAND(list` in console.cpp.
	// list_cmd_f uses private members to iterate the console_command linked list.
	friend static void list_cmd_f(command_args& args);

	// Set the `open` variable to 0.
	// Join the other thread to this one.
	~console();
	// Create input thread
	console();

	// queues a command to the main thread
	// this makes commands managing the server thread safe
	// TODO make executing thread tokenize into arguments
	static void exec(const char* str);
	// execute and flush commands
	static void flush();
	
	// We use these methods so that the mutex is unlocked after accessing
	bool is_open();
	void set_open(bool open);

private:
	HANDLE						_hinput;

	INPUT_RECORD				_record[64];
	std::vector<std::string>	_inputrecord;

	std::thread					_thread;
	char						_input[64];

	bool						_open;
	std::mutex					_open_mut;

	size_t						_cursor;
	size_t						_line;	// Line number

	void uparrow();
	void downarrow();
	void leftarrow();
	void rightarrow();
	void newline();
	void backspace();
	void tab();
	void onchar(char ch);

	static void console_thread(console* con);

	static console_command* find_cmd(const char* name);

	static console_command*			_head;

	static std::queue<command_args> _queue;
	static std::mutex				_queue_mut;
};