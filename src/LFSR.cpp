/*

LFSR group. Linear Feedback Shift Register sequencers for VCV Rack

Robert A Moeser

Use however you want, but it would be better if you didn't learn anything from
this. I am not a C++ programmer, I am not a musician.

If you do make things starting here, leave my name around. My mom likes that.

*/


#include "LFSR.hpp"

Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	p->slug = TOSTRING("LFSR");
#ifdef VERSION
	p->version = TOSTRING(VERSION);
#endif

	p->website = "https://github.com/alto777/LFSR";
	p->manual = "https://github.com/alto777/LFSR/README.md";

	p->addModel(modelFG8);
}
