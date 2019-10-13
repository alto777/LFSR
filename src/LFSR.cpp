/*

LFSR group. Linear Feedback Shift Register sequencers for VCV Rack

Robert A Moeser

Use however you want, but it would be better if you didn't learn anything from
this. I am not a C++ programmer, I am not a musician.

If you do make things starting here, leave my name around. My mom likes that.

*/


#include "LFSR.hpp"

Plugin *pluginInstance;

void init(rack::Plugin *p) {
	pluginInstance = p;

	p->addModel(modelFG8);
	p->addModel(modelPsychtone);

 	p->addModel(modelAmuse);
	p->addModel(modela7Utility);
	p->addModel(modelcheapFX);
	p->addModel(modelDivada);
	p->addModel(modelYASeq3);
}
