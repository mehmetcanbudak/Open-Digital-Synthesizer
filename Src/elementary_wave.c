/*
 * elementary_wave.c
 *
 *  Created on: May 28, 2020
 *      Author: omurakosuke
 */

#include "elementary_wave.h"

void add_square(float *_audio, uint32_t _start_index, float _period_us,
		uint16_t _amp, uint16_t _size) {
	if (_amp == 0) {
		return;
	}

	float audio_temp[NUM_SAMPLING] = { 0 };
	float half_period = _period_us / 2;
	uint32_t half_cycle_count = (uint32_t)((SAMPLE_CYCLE * _start_index) / half_period);

	for(uint16_t t = 0; t < _size; t++){
		float time_us = SAMPLE_CYCLE * (t + _start_index);

		if((half_cycle_count % 2) == 0){
			audio_temp[t] = _amp;
		} else {
			audio_temp[t] = -_amp;
		}

		if((time_us + SAMPLE_CYCLE) >= ((half_cycle_count + 1) * half_period)){
			half_cycle_count++;
		}
	}

	arm_add_f32((_audio + PRE_SAMPLE), audio_temp, (_audio + PRE_SAMPLE), NUM_SAMPLING);

	return;
}

void add_triangle(float *_audio, uint32_t _start_index, float _period_us,
		uint16_t _amp, uint16_t _size) {
	if (_amp == 0) {
		return;
	}

	float audio_temp[NUM_SAMPLING] = { 0 };
	float half_period = _period_us / 2;
	uint32_t half_cycle_count = (uint32_t)((SAMPLE_CYCLE * _start_index) / half_period);
	float slope = 2 * _amp / half_period;

	for(uint16_t t = 0; t < _size; t++){
		float time_us = SAMPLE_CYCLE * (t + _start_index);

		if((half_cycle_count % 2) == 0){
			audio_temp[t] = -_amp + slope * (time_us - (half_period * half_cycle_count));
		} else {
			audio_temp[t] = _amp - slope * (time_us - (half_period * half_cycle_count));
		}

		if((time_us + SAMPLE_CYCLE) >= ((half_cycle_count + 1) * half_period)){
			half_cycle_count++;
		}
	}

	arm_add_f32((_audio + PRE_SAMPLE), audio_temp, (_audio + PRE_SAMPLE), NUM_SAMPLING);

	return;
}

void add_sawtooth(float *_audio, uint32_t _start_index, float _period_us,
		uint16_t _amp, uint16_t _size) {
	if (_amp == 0) {
		return;
	}

	float audio_temp[NUM_SAMPLING] = { 0 };
	uint32_t cycle_count = (uint32_t)((SAMPLE_CYCLE * _start_index) / _period_us);
	float slope = 2 * _amp / _period_us;

	for(uint16_t t = 0; t < _size; t++){
		float time_us = SAMPLE_CYCLE * (t + _start_index);
		audio_temp[t] = -_amp + slope * (time_us - (_period_us * cycle_count));

		if((time_us + SAMPLE_CYCLE) >= ((cycle_count + 1) * _period_us)){
			cycle_count++;
		}
	}

	arm_add_f32((_audio + PRE_SAMPLE), audio_temp, (_audio + PRE_SAMPLE), NUM_SAMPLING);

	return;
}

void add_sin(float *_audio, uint32_t _start_index, float _freq, uint16_t _amp, uint16_t _size) {
	if (_amp == 0) {
		return;
	}

	float audio_temp[NUM_SAMPLING] = { 0 };
	float omega = 2 * M_PI * _freq * (SAMPLE_CYCLE/ (float)1000000);
	for(uint16_t t = 0; t < _size; t++){
		uint32_t time_index = t + _start_index;
		audio_temp[t] = arm_sin_f32(omega * time_index);
	}

	arm_scale_f32(audio_temp, _amp, audio_temp, NUM_SAMPLING);
	arm_add_f32((_audio + PRE_SAMPLE), audio_temp, (_audio + PRE_SAMPLE), NUM_SAMPLING);

	return;
}

//
void float2uint(float *_in, uint16_t *_out, uint16_t _start, uint16_t _end) {
	float tmp;
	for (uint16_t t = _start; t < _end; t++) {
		tmp = _in[t + PRE_SAMPLE];

		if(tmp < 0){
			tmp = 0;
		}else if(tmp > AMP_MAX){
			tmp = AMP_MAX;
		}
		_out[t] = (uint16_t)tmp;
	}
}

void calc_lpf_coeffs(float32_t *_coeffs, float _freq_cut, float _q_factor) {
	float omega_c = 2 * M_PI * _freq_cut / SAMPLE_FREQ;
	float alfa = arm_sin_f32(omega_c) / _q_factor;
	float a0 = 1 + alfa;
	float cos_omega_c = arm_cos_f32(omega_c);

	_coeffs[1] = (1 - cos_omega_c) / a0; //b1
	_coeffs[0] = _coeffs[1] / 2; //b0
	_coeffs[2] = _coeffs[0]; //b2
	_coeffs[3] = (2 * cos_omega_c) / a0; //a1
	_coeffs[4] = 1 - (2 / a0); //a2
}

void calc_amp_char(uint16_t *_amp, float _freq_cut, float _q_factor) {
	float omega_c = 2 * M_PI * (float) _freq_cut;
	float inv_q_sqare = 1 / (_q_factor * _q_factor);

	for (uint16_t i = 0; i < 240; i++) {
		float omega = log_axis[i];
		float omega_c_per = omega_c / omega;
		float omega_c_per_square = omega_c_per * omega_c_per;
		float sqrt_in, amp_temp;

		sqrt_in = omega_c_per_square + (1 / omega_c_per_square) + inv_q_sqare
				- 2;
		arm_sqrt_f32(sqrt_in, &amp_temp);
		amp_temp = omega_c_per / amp_temp;
		amp_temp = -20 * log10f(amp_temp) + 20;
		_amp[i] = (uint16_t) amp_temp;
	}
}

//------------
const float log_axis[240] = { 125.663706143592, 129.348728894708,
		133.141813019254, 137.046127360741, 141.064933687363, 145.201589416971,
		149.459550421943, 153.842373916319, 158.353721427585, 162.997361855615,
		167.777174621311, 172.697152907572, 177.761406995305, 182.974167697266,
		188.339789892592, 193.862756164982, 199.547680547569, 205.399312377606,
		211.422540264186, 217.622396172319, 224.00405962677, 230.572862039168,
		237.334291162016, 244.293995673299, 251.457789895542, 258.831658653253,
		266.421762272802, 274.23444172892, 282.276223942122, 290.55382723147,
		299.074166927232, 307.844361148136, 316.871736748036, 326.16383543696,
		335.728420081655, 345.573481190894, 355.707243590959, 366.138173296881,
		376.874984585173, 387.926647273968, 399.302394216647, 411.011729015209,
		423.064433959833, 435.470578201262, 448.240526162845, 461.384946199247,
		474.914819509078, 488.841449308881, 503.17647027614, 517.931858269193,
		533.119940332195, 548.753404993447, 564.845312865737, 581.409107557524,
		598.458626904083, 616.008114528006, 634.072231738697, 652.666069780829,
		671.805162441964, 691.505499029896, 711.783537730546, 732.656219357569,
		754.140981505155, 776.255773115868, 799.019069475666, 822.449887648646,
		846.56780236441, 871.392962371313, 896.946107269263, 923.248584836137,
		950.322368862284, 978.190077508011, 1006.8749921994, 1036.40107707824,
		1066.79299902227, 1098.07614825262, 1130.27665954539, 1163.42143406542,
		1197.53816184017, 1232.65534489275, 1268.80232105327, 1306.0092884684,
		1344.30733082978, 1383.72844334208, 1424.30555945272, 1466.0725783653,
		1509.06439335992, 1553.31692094395, 1598.86713085763, 1645.7530769596,
		1694.01392901807, 1743.69000543432, 1794.82280692576, 1847.45505119672,
		1901.63070862596, 1957.39503900072, 2014.79462932791, 2073.87743275419,
		2134.69280862726, 2197.29156373198, 2261.72599473576, 2328.0499318785,
		2396.31878394389, 2466.58958454936, 2538.92103979343, 2613.37357730044,
		2690.00939670333, 2768.89252160687, 2850.08885307474, 2933.66622468498,
		3019.69445919998, 3108.24542689828, 3199.39310561694, 3293.21364255455,
		3389.78541788666, 3489.18911024665, 3591.50776412681, 3696.82685925595,
		3805.23438201139, 3916.82089892518, 4031.67963234573, 4149.90653831832,
		4271.60038674925, 4396.86284392094, 4525.79855742656, 4658.5152435955,
		4795.12377748239, 4935.73828549514, 5080.47624073912, 5229.45856115735,
		5382.80971054859, 5540.65780254767, 5703.13470765507, 5870.37616340502,
		6042.52188776427, 6219.71569585619, 6402.10562010778, 6589.84403391985,
		6783.08777896388, 6981.99829621169, 7186.74176080753, 7397.48922089525,
		7614.41674051642, 7837.705546699, 8067.54218085915, 8304.11865464295,
		8547.63261033799, 8798.28748598908, 9056.29268535572, 9321.86375285365,
		9595.22255362638, 9876.59745889721, 10166.2235367566, 10464.3427485446,
		10771.2041509912, 11087.0641042856, 11412.186486246, 11746.8429127701,
		12091.3129647506, 12445.8844216441, 12810.85350189, 13186.5251103792,
		13573.2130931798, 13971.2404997323, 14380.939852734, 14802.6534259371,
		15236.7335300933, 15683.5428072837, 16143.4545338796, 16616.8529323876,
		17104.1334924391, 17605.7033011931, 18121.9813834276, 18653.399051604,
		19200.400266197, 19763.4420065911, 20342.9946528535, 20939.5423787015,
		21553.583555995, 22185.6311710889, 22836.213253396, 23505.8733165167,
		24195.1708123047, 24904.6815982479, 25634.9984185554, 26386.7313993514,
		27160.5085583912, 27956.9763297236, 28776.8001037393, 29620.6647830559,
		30489.2753547035, 31383.3574790898, 32303.6580962362, 33250.9460497912,
		34226.012729343, 35229.6727315675, 36262.7645407638, 37326.1512293466,
		38420.7211788797, 39547.3888222538, 40707.0954076278, 41900.809784773,
		43129.5292144756, 44394.2802016752, 45696.1193530341, 47036.134259655,
		48415.4444056833, 49835.202103554, 51296.5934566645, 52800.8393502764,
		54349.1964714763, 55942.9583590447, 57583.4564841139, 59272.0613625136,
		61010.1836997366, 62799.2755694798, 64640.831626745, 66536.390356514,
		68487.5353590394, 70495.8966728266, 72563.1521364116, 74691.0287900718,
		76881.3043186414, 79135.8085366368, 81456.4249169323, 83845.0921642637,
		86303.8058348733, 88834.6200036517, 91439.6489801656, 94121.0690750094,
		96881.120417952, 99722.1088294016, 102646.407746749, 105656.460207202,
		108754.78088876, 111943.958211047, 115226.656497745, 118605.61820244,
		122083.666199741, 125663.706143586
};
