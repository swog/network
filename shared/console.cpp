#include "stdafx.h"
#include <conio.h>
#include "console.h"

#ifdef _SERVER
#include "server.h"

// Executing from rcon?
// Security feature to hide commands executed by the server
// E.g.: rcon_password, to secure the server from any rcon hackers
static std::mutex _con_rcon_exec_mut;
static bool _con_rcon_exec = false;
// Queue, if there are multiple rcon executers
static std::queue<std::weak_ptr<client>> _con_log;

// Set log client thru svc_print
void sv_con_log(std::shared_ptr<client>& cl) {
	_con_log.push(cl);
}

void sv_con_unlog() {
	if (_con_log.size())
		_con_log.pop();
}

void sv_con_setrcon(bool rcon) {
	std::lock_guard<std::mutex> lock(_con_rcon_exec_mut);
	_con_rcon_exec = rcon;
}

static bool sv_con_isrcon() {
	std::lock_guard<std::mutex> lock(_con_rcon_exec_mut);
	return _con_rcon_exec;
}
#endif

// This method provides logging functionality for echoing to client
void con_printf(const char* format, ...) {
	static char text[1024];
	va_list ap;
	va_start(ap, format);
	vsprintf_s(text, format, ap);
	vfprintf(stdout, format, ap);
	va_end(ap);

	// Log to the executing rcon client
	// BUG: It is possible that another rcon client comes through
#ifdef _SERVER
	// Do not log if its not an rcon executed command
	if (!sv_con_isrcon())
		return;

	auto& sv = get_server();

	// Lock the shared ptr from the front of the rcon console log recipients
	// AKA. the first log recipient client.
	std::shared_ptr<client> cl;
	if (_con_log.size()) {
		cl = _con_log.front().lock();
		if (cl) {
			auto s = cl->stream();
			s << svc_print;
			s << (const char*)text;
			s.flush();
			// HACK
			// Output is too fast because we don't have any buffered stream output
			Sleep(1);
		}
	}
#endif
}

CONSOLE_COMMAND(list, "List all available commands", 0) {
	con_printf("Commands:\n");
	// Width is unsigned char so that an attacker somehow could 
	//	create a command whose name length overflows format_string.
	// "%255s - %s" is not greater than 31 characters.
	unsigned char width = 0, len;
	for (console_command* com = console::_head; com; com = com->_next) {
		len = (unsigned char)strlen(com->name());
		if (len > width)
			width = len;
	}
	static char format_string[32];
	sprintf_s(format_string, "%%%us - %%s\n", width);
	for (console_command* com = console::_head; com; com = com->_next) {
		con_printf(format_string, com->name(), com->help());
	}
}

// Echo command to print a string to console
CONSOLE_COMMAND(echo, "Print to console", 0) {
	con_printf("%s\n", args.argstr.c_str());
}

//=============================================================================
// 
// Console command argument parser
// 
// 1. Converts the input ascii into utf16. 
// 2. Creates standardized argc, argv using windows api's CommandLineToArgvW
// 3. Delete the input utf16 resource.
// 4. Convert each utf16 arg into char* resource, push it into argv, then delete char*.
// 5. Find and execute the command, or output unknown command.
// 6. FreeResource the wchar_t argvw heap lump
// 
//=============================================================================
command_args::command_args(const char* str) {
	// number of converted chars
	size_t num = 0,
		// size of string
		size = strlen(str) + 1;
	// cmdstr input wcs
	wchar_t* pwc = new wchar_t[size];
	// convert to widechar
	// Multibyte String To Wide Char String Safe
	mbstowcs_s(&num, pwc, size, str, size - 1);

	int argc = 0;
	// wide argv from shellapi
	wchar_t** argvw = CommandLineToArgvW(pwc, &argc);
	// delete the input string
	delete[] pwc;

	// temp str to convert wide chars from CommandLineToArgvW
	char* temp;

	for (int i = 0; i < argc; i++) {
		size = wcslen(argvw[i]) + 1;
		temp = new char[size];
		wcstombs_s(&num, temp, size, argvw[i], size - 1);
		argv.push_back(temp);
		delete[] temp;
	}

	argstr = str;
	cmd = NULL;

	if (argv.size() && argv[0].size()) {
		cmd = console::find_cmd(argv[0].c_str());
		if (!cmd)
			con_printf("Unknown command: %s\n", argv[0].c_str());
		if (argstr.size() > argv[0].size() + 1)
			argstr = str + argv[0].size() + 1;
	}

	FreeResource(argvw);
}

//=============================================================================
// 
// Console command section
// 
//=============================================================================
console_command* console::_head = NULL;

void console_command::init(const char* name, const char* help, size_t flags, console_commandfn fn) {
	_name = name;
	_help = help;
	_flags = flags;
	_fn = fn;
	_next = console::_head;
	console::_head = this;
}

console_command* console::find_cmd(const char* name) {
	for (console_command* cmd = _head; cmd; cmd = cmd->_next)
		if (!strcmp(cmd->name(), name))
			return cmd;
	return NULL;
}

console_command::console_command() {
	_name = NULL;
	_help = NULL;
	_flags = 0;
	_fn = NULL;
	_next = NULL;
}

//=============================================================================
// 
// Console variable section
// 
//=============================================================================
static void console_var_f(command_args& args);
console_commandfn console_var::_varfn = console_var_f;

console_var::console_var(const char* name, const char* help, size_t flags, const char* val, console_var_changefn changefn) {
	_changefn = changefn;
	set_value(val);
	init(name, help, flags, _varfn);
}

console_var::console_var(const char* name, const char* help, size_t flags, int num, console_var_changefn changefn) {
	_changefn = changefn;
	set_value(num);
	init(name, help, flags, _varfn);
}

console_var::console_var(const char* name, const char* help, size_t flags, float fl, console_var_changefn changefn) {
	_changefn = changefn;
	set_value(fl);
	init(name, help, flags, _varfn);
}

void console_var::set_value(const std::string& str) {
	auto c_str = str.c_str();
	int num = atoi(c_str);
	float fl = (float)atof(c_str);
	if (_changefn && 
		(strcmp(_str.c_str(), c_str) || 
		_num != num ||
		_fl != fl)
	) {
		_changefn(*this, c_str, num, fl);
	}
	_str = str;
	_num = num;
	_fl = fl;
}

static void console_var_f(command_args& args) {
	// Weird error where a command has the callback for a variable,
	//	but is not a variable
	if (!args.cmd->is_var()) {
		con_printf("Console command %s has varfn but is not var!\n", args.cmd->name());
		return;
	}

	auto& var = (console_var&)*args.cmd;

	if (args.argv.size() > 1) {
		var.set_value(args.argv[1]);
	}
	else {
		// This is the only way to access the value of a variable as of now.
		// Hide special ones.
		if (var.flags() & fcommand_hidevalue)
			con_printf("%s = \"****\"\n", var.name());
		else
			con_printf("%s = \"%s\"\n", var.name(), var.get_cstr());
	}
}

//=============================================================================
// 
// Console instance section
// 
//=============================================================================
std::vector<command_args> console::_queue;

// Set the `open` variable to 0.
// Join the other thread to this one.
console::~console() {
	set_open(0);
	_thread.join();
}

// Create input thread
console::console() {
	_hinput = GetStdHandle(STD_INPUT_HANDLE);

	_line = 0;
	_open = 1;
	_cursor = 0;
	_input[0] = 0;

	_thread = std::thread(console_thread, this);
}

// We use these methods so that the mutex is unlocked after accessing
bool console::is_open() {
	std::lock_guard<std::mutex> lock(_open_mut);
	return _open;
}

void console::set_open(bool open) {
	std::lock_guard<std::mutex> lock(_open_mut);
	_open = open;
}

void console::uparrow() {
	// reset the line to the input string
	// then clear it
	// this allows the actual cursor to be at the end before printing backspaces
	for (size_t i = _cursor; i < strlen(_input); i++)
		printf("%c", _input[i]);

	for (size_t i = 0; i < strlen(_input); i++)
		printf("\b \b");

	// underflow check
	if (!_line) {
		_cursor = 0;
		_input[0] = 0;
		_line = _inputrecord.size();
		return;
	}

	_line--;

	strcpy_s(_input, _inputrecord[_line].c_str());
	printf("%s", _input);

	_cursor = strlen(_input);
}

void console::downarrow() {
	// reset the line to the input string
	// then clear it
	// this allows the actual cursor to be at the end before printing backspaces
	for (size_t i = _cursor; i < strlen(_input); i++)
		printf("%c", _input[i]);

	for (size_t i = 0; i < strlen(_input); i++)
		printf("\b \b");

	if (_line + 1 >= _inputrecord.size()) {
		_cursor = 0;
		_input[0] = 0;
		_line = _inputrecord.size();
		return;
	}

	_line++;

	strcpy_s(_input, _inputrecord[_line].c_str());
	printf("%s", _input);

	_cursor = strlen(_input);
}

void console::leftarrow() {
	if (!_cursor)
		return;
	
	printf("\b");
	_cursor--;
}

void console::rightarrow() {
	if (_cursor >= strlen(_input))
		return;
	
	printf("%c", _input[_cursor]);
	_cursor++;
}

void console::newline() {
	printf("\n");

	_inputrecord.push_back(_input);
	_line++;

	printf("> %s\n", _input);
	exec(_input);

	_cursor = 0;
	_input[0] = 0;
}

void console::backspace() {
	if (!_cursor)
		return;
	
	for (size_t i = _cursor; i < strlen(_input); i++)
		printf("%c", _input[i]);

	for (size_t i = 0; i < strlen(_input); i++)
		printf("\b \b");

	memmove(_input + _cursor - 1, _input + _cursor, sizeof(_input) - _cursor);
	printf("%s", _input);

	_cursor--;

	size_t len = strlen(_input);

	if (len) {
		for (size_t i = len - 1; i >= _cursor; i--)
			printf("\b");
	}

	_line = _inputrecord.size();
}

void console::tab() {
	static std::string cmdname;
	cmdname.clear();
	for (size_t i = 0; i < strlen(_input) && 
		i < sizeof(_input) && _input[i] != ' '; i++)
		cmdname.push_back(_input[i]);

	for (size_t i = 0; i < strlen(_input); i++)
		printf("\b \b");

	size_t len = cmdname.size();

	for (console_command* cmd = _head; cmd; cmd = cmd->_next) {
		if (!strncmp(cmdname.c_str(), cmd->name(), len)) {
			strcpy_s(_input, cmd->name());
			_cursor = strlen(_input);
			break;
		}
	}

	printf("%s", _input);
}

void console::onchar(char ch) {
	if (_cursor >= sizeof(_input) - 1)
		return;
	
	memmove(_input + _cursor + 1, _input + _cursor, sizeof(_input) - _cursor - 1);
	_input[_cursor] = ch;

	for (size_t i = 0; i < strlen(_input); i++)
		printf("\b \b");
	printf("%s", _input);
	for (size_t i = strlen(_input) - 1; i != _cursor; i--)
		printf("\b");

	_cursor++;
	_line = _inputrecord.size();
}

void console::console_thread(console* con) {
	DWORD numevents, numread;
	PINPUT_RECORD rec;
	char ch;

	while (con->is_open()) {
		if (!GetNumberOfConsoleInputEvents(con->_hinput, &numevents) || !numevents)
			continue; 
		if (!ReadConsoleInputA(con->_hinput, con->_record, ARRAYSIZE(con->_record), &numread) || !numread)
			continue;
		for (size_t i = 0; i < numread; i++) {
			rec = con->_record + i;
			if (rec->EventType != KEY_EVENT)
				continue;
			if (rec->Event.KeyEvent.bKeyDown) {
				if (rec->Event.KeyEvent.wVirtualKeyCode == VK_UP)
					con->uparrow();
				else if (rec->Event.KeyEvent.wVirtualKeyCode == VK_DOWN)
					con->downarrow();
				else if (rec->Event.KeyEvent.wVirtualKeyCode == VK_LEFT)
					con->leftarrow();
				else if (rec->Event.KeyEvent.wVirtualKeyCode == VK_RIGHT)
					con->rightarrow();
				else {
					ch = rec->Event.KeyEvent.uChar.AsciiChar;

					switch (ch) {
					case '\r':
						con->newline();
						break;
					case '\b':
						con->backspace();
						break;
					case '\t':
						con->tab();
						break;
					default:
						if (ch >= ' ' && ch <= '~')
							con->onchar(ch);
					}
				}
			}
		}
	}
}

// Tokenizes command string and queues to main thread
void console::exec(const char* cmdstr) {
	// Tokenize in this thread
	command_args args(cmdstr);
	// If the command is found, queue it
	if (args.cmd)
		_queue.push_back(args);
}

// Execute and flush commands
// Called from the main thread
void console::flush() {
	for (auto& args : console::_queue)
		args.cmd->execute(args);
	console::_queue.clear();
#ifdef _SERVER
	if (sv_con_isrcon()) {
		sv_con_setrcon(0);
		sv_con_unlog();
	}
#endif
}
