# include "LFSR.hpp"

/* at x1 works good except switching noise */
/* at x2 noise gone, but we get the tail end effect, a wee bit of f0 */
/* at 4. total rewrite ARGH! works good except I still get the tail effect */

//# define FADE_STEPS	20000	/* whoa! */
# define FADE_STEPS	2000

struct cheapFX : Module {
	enum ParamIds {
		ENUMS(FREQUENCY_PARAM, 2),
		ENUMS(SHAPE_PARAM, 2),

		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(FREQUENCY_CV_INPUT, 2),
		ENUMS(SHAPE_CV_INPUT, 2),
		ENUMS(TRIGGER_INPUT, 2),

		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(TRIANGLE_OUTPUT, 2),
		ENUMS(RECTANGLE_OUTPUT, 2),
		ENUMS(GATE_OUTPUT, 2),

		NUM_OUTPUTS
	};
	enum LightIds {

		NUM_LIGHTS
	};

	enum fadeFSMStates {
		OFF, START, COMING, ON, STOP, GOING
	};

	enum tweedFSMStates {
		NOT, INIT, TWEEDLE, FINI, BUSY
	};

	float phase[2] = {0.0f, 0.0f};
	dsp::SchmittTrigger eventTrigger[2];
	bool isGated[2] = {0, 0};
	bool phaseRolloverFlag[2] = {0, 0};
	bool retriggerFlag[2] = {0, 0};

	int tCount[2] = {0, 0};

	int  fadeCount[2];	/* for soft gate process */
	fadeFSMStates fadeState[2] = {OFF, OFF};
	tweedFSMStates tweedState[2] = {NOT, NOT};

	bool runnable = 0;

/* overkill for simple intialization, yet my best way of sandwiching it on in there */
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		if (!runnable) {
			for (int ii = 0; ii < 2; ii++) {
				phase[ii] = 0.0f;
// 			previousGate[ii] = 0.0f;
				isGated[ii] =0;
				tCount[ii] = 0;
				fadeState[ii] = OFF;
				tweedState[ii] = NOT;
				phaseRolloverFlag[ii] = 0;
				retriggerFlag[ii] = 0;
			}
			runnable = 1;
		}
		return (rootJ);
	}

	void dataFromJson(json_t *rootJ) override {
		for (int ii = 0; ii < 2; ii++) {

		}
	}

	cheapFX() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int i = 0; i < 2; i++) {
			configParam(cheapFX::FREQUENCY_PARAM + i, -3.0, 3.0, 0.0, "");
			configParam(cheapFX::SHAPE_PARAM + i, 0.0, 1.0, 0.5, "");
		}
	}
	void process(const ProcessArgs& args) override;
};

struct myBoltA : SvgScrew {
	myBoltA() {
		sw->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/myBoltA.svg")));
		box.size = sw->box.size;
	}
};

struct myBoltB : SvgScrew {
	myBoltB() {
		sw->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/myBoltB.svg")));
		box.size = sw->box.size;
	}
};

struct myBoltC : SvgScrew {
	myBoltC() {
		sw->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/myBoltC.svg")));
		box.size = sw->box.size;
	}
};

struct myBoltD : SvgScrew {
	myBoltD() {
		sw->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/myBoltD.svg")));
		box.size = sw->box.size;
	}
};

void cheapFX::process(const ProcessArgs& args) {
/* first - a square wave at frequency see Template.cpp */
	float deltaTime = args.sampleTime;

  for (int ii = 0; ii < 2; ii++) {	/* [ii] */
// ram pup or down the gate output for softer transitions
/* inline because I am not confidant about scope and stuff. Sad. */
	switch (fadeState[ii]) {
	case OFF :			/* we dead, kill it again */
		outputs[GATE_OUTPUT + ii].setVoltage(0.0f);
		break;

	case ON :			/* we full on */
		outputs[GATE_OUTPUT + ii].setVoltage(10.0f);
		break;

	case START :		/* externally kicked */
		fadeCount[ii] = 0;
		fadeState[ii] = COMING;
		[[fallthrough]];
		/* break not, get right to it */
	case COMING :
		if (fadeCount[ii] < FADE_STEPS) {
			++fadeCount[ii];
			outputs[GATE_OUTPUT + ii].setVoltage((10.0f * fadeCount[ii]) / FADE_STEPS);
		}
		else fadeState[ii] = ON;
		break;

	case STOP :			/* also externally kicked */
		fadeState[ii] = GOING;
		[[fallthrough]];
		/* break not, get right to it */
	case GOING :
		if (fadeCount[ii] > 0) {
			--fadeCount[ii];
			outputs[GATE_OUTPUT + ii].setVoltage((10.0f * fadeCount[ii]) / FADE_STEPS);
		}
		else fadeState[ii] = OFF;
		break;
	}

	switch (tweedState[ii]) {
	case INIT :
	/* when we start a tweedle cycle */
		phase[ii] = 0.0f;		/* restart the tweedle waveform */
		phaseRolloverFlag[ii] = 0;
		retriggerFlag[ii] = 0;
		fadeState[ii] = START;	/* send in the clowns */
		/* break not, get going right now */
		tweedState[ii] = TWEEDLE;
		[[fallthrough]];

	case TWEEDLE :
	/* this carries out one full tweedle, maybe retriggers/stays on */
		if (eventTrigger[ii].process(inputs[TRIGGER_INPUT + ii].getVoltage()))
			retriggerFlag[ii] = 1;	/* any trigger during buys another cycle */

		if (phaseRolloverFlag[ii]) {
			if (retriggerFlag[ii] || eventTrigger[ii].isHigh()) {
				retriggerFlag[ii] = 0;
				phaseRolloverFlag[ii] = 0;
				/* state remains the same */
			}
			else {	/* we've finished a tweedle and should stop it */
				tweedState[ii] = FINI;
				break;
			}
		}
		[[fallthrough]];
		/* break not, NOT is just what we need to do anyway */
		/* but there's a slight chance to see a trigger, so if around that by state YUCK! */
	case NOT :
		if (tweedState[ii] == NOT) {
			if (eventTrigger[ii].process(inputs[TRIGGER_INPUT + ii].getVoltage())) {
				tweedState[ii] = INIT;	/* and get right to it */
				break;
			}
		}
		if (1) {	/* cannot jump from switch statement to this case label */
		float pitch = params[FREQUENCY_PARAM + ii].getValue();
		pitch += inputs[FREQUENCY_CV_INPUT + ii].getVoltage();
		pitch = clamp(pitch, -4.0f, 4.0f);

		float freq = 2.04395f * powf(2.0f, pitch);		// LFO, right?

//--> don't advance phase in the middle of a fade out on the gate
// ARGH! too late, the phase done rolled over, that's what kicked us
//		if ((fadeState[ii] == ON) || (fadeState[ii] == OFF))

			phase[ii] += freq * deltaTime;

		if (phase[ii] >= 1.0f) {
			phaseRolloverFlag[ii] = 1;
			phase[ii] -= 1.0f;
		}
	/* YIKES !!! */
		if ((tweedState[ii] == TWEEDLE) || (tweedState[ii] == NOT)) {
			float x = params[SHAPE_PARAM + ii].getValue() + inputs[SHAPE_CV_INPUT + ii].getVoltage() * 0.1f;
			float t = phase[ii];		/* hey! */
			float y;

/* yuck. skip one sample update at phase rollover. FIGURE THIS AWAY! */
			if (!phaseRolloverFlag[ii])
				outputs[RECTANGLE_OUTPUT + ii].setVoltage((phase[ii] > x) ? 0.0f : 10.0f);

			if (x >= 1.0f) y = t;
			else if(x <= 0.0f) y = 1.0f - t;
			else if (t < x) y = t / x;
			else y = (1.0f - t) / (1.0f - x);

			if (!phaseRolloverFlag[ii])
				outputs[TRIANGLE_OUTPUT + ii].setVoltage(10.0f * y);
		}
		}	/* if (1) */
		break;

	case FINI :
		fadeState[ii] = STOP;
		tweedState[ii] = BUSY;
		break;

	case BUSY :
		if (fadeState[ii] == OFF) tweedState[ii] = NOT;
		break;
	}
  }
}

struct cheapFXWidget : ModuleWidget {
  cheapFXWidget(cheapFX *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/cheapFX.svg")));

		addChild(createWidget<myBoltA>(Vec(0, 0)));
		addChild(createWidget<myBoltB>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
		addChild(createWidget<myBoltD>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<myBoltC>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));


		addParam(createParam<RoundLargeBlackKnob>(mm2px(Vec(2.4, 12.845)), module, cheapFX::FREQUENCY_PARAM + 0));
		addParam(createParam<RoundLargeBlackKnob>(mm2px(Vec(20.461, 12.845)), module, cheapFX::SHAPE_PARAM + 0));
		addParam(createParam<RoundLargeBlackKnob>(mm2px(Vec(2.4, 72.641)), module, cheapFX::FREQUENCY_PARAM + 1));
		addParam(createParam<RoundLargeBlackKnob>(mm2px(Vec(20.461, 72.641)), module, cheapFX::SHAPE_PARAM + 1));

		addInput(createInput<PJ301MPort>(mm2px(Vec(4.572, 28.358)), module, cheapFX::FREQUENCY_CV_INPUT + 0));
		addInput(createInput<PJ301MPort>(mm2px(Vec(22.449, 28.359)), module, cheapFX::SHAPE_CV_INPUT + 0));
		addInput(createInput<PJ301MPort>(mm2px(Vec(4.572, 53.229)), module, cheapFX::TRIGGER_INPUT + 0));
		addInput(createInput<PJ301MPort>(mm2px(Vec(4.572, 88.154)), module, cheapFX::FREQUENCY_CV_INPUT + 1));
		addInput(createInput<PJ301MPort>(mm2px(Vec(22.449, 88.155)), module, cheapFX::SHAPE_CV_INPUT + 1));
		addInput(createInput<PJ301MPort>(mm2px(Vec(4.572, 113.024)), module, cheapFX::TRIGGER_INPUT + 1));

		addOutput(createOutput<PJ301MPort>(mm2px(Vec(4.572, 40.11)), module, cheapFX::TRIANGLE_OUTPUT + 0));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(22.633, 40.111)), module, cheapFX::RECTANGLE_OUTPUT + 0));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(22.633, 53.229)), module, cheapFX::GATE_OUTPUT + 0));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(4.572, 99.906)), module, cheapFX::TRIANGLE_OUTPUT + 1));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(22.633, 99.907)), module, cheapFX::RECTANGLE_OUTPUT + 1));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(22.633, 113.024)), module, cheapFX::GATE_OUTPUT + 1));
  }
};

Model *modelcheapFX = createModel<cheapFX, cheapFXWidget>("cheapFX");
