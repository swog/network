#include "stdafx.h"
#include "whitelist.h"
#include <fstream>

static std::vector<std::string> _whitelist;
static bool _enable_whitelist = 0;
static bool _should_init = 1;

void sv_enablewhitelist() {
	_enable_whitelist = 1;
}

static void whitelist_init() {
	_should_init = 1;

	std::ifstream ifs("whitelist.txt");
	static char line[64];

	line[63] = 0;

	while (ifs.good()) {
		ifs.getline(line, sizeof(line) - 1);
		_whitelist.push_back(line);
	}

	ifs.close();
}

bool sv_inwhitelist(const char* addr) {
	if (!_enable_whitelist)
		return 1;

	if (_should_init) {
		whitelist_init();
	}

	for (const auto& ip : _whitelist) {
		if (!ip.compare(0, ip.size(), addr))
			return 1;
	}
	return 0;
}
