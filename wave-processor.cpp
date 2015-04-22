#include <io.h>
// for _O_RDONLY etc.
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <FLOAT.H>
#include <vector>
#include "wave-processor.h"

typedef struct {
	char			data_chunk_id[4];	/* 'data' */
	unsigned long	data_chunk_size;	/* length of data */
} waveheader_ext_t;


WaveProcessor::WaveProcessor(int unifiedSamplingRate)
{
	m_unifiedSamplingRate = unifiedSamplingRate;
	m_pfWaveR = NULL;
	memset(m_newlyMadeSamples, 0, (sizeof(short) + 4) * BUFFER_SIZE_IN_SAMPLES_DS);
	coefficient[0] = 0.003638091;
	coefficient[1] = 0.008158223;
	coefficient[2] = 0.020889028;
	coefficient[3] = 0.043770138;
	coefficient[4] = 0.074889944;
	coefficient[5] = 0.108669917;
	coefficient[6] = 0.137467074;
	coefficient[7] = 0.154050524;
	coefficient[8] = 0.154050524;
	coefficient[9] = 0.137467074;
	coefficient[10] = 0.108669917;
	coefficient[11] = 0.074889944;
	coefficient[12] = 0.043770138;
	coefficient[13] = 0.020889028;
	coefficient[14] = 0.008158223;
	coefficient[15] = 0.003638091;
}

WaveProcessor::~WaveProcessor()
{
	if (m_pfWaveR)
		fclose(m_pfWaveR);
}

void WaveProcessor::CloseWaveFile()
{
	if (m_pfWaveR) {
		fclose(m_pfWaveR);
		m_pfWaveR = NULL;
	}
}

void WaveProcessor::Clear()
{
	m_pfWaveR = NULL;
	memset(m_newlyMadeSamples, 0, (sizeof(short) + 4) * BUFFER_SIZE_IN_SAMPLES_DS);
	memset(samplesArray, 0, sizeof(short)* SamplesVectorSize);
}

// 读文件头，并检查文件是否合法
// 最后两个参数：消息缓存指针和缓存大小（字节数）
// 
int WaveProcessor::OpenWaveFile(const char *psfn_waveR)
{
	m_pfWaveR = NULL;

	FILE *pfWave = fopen(psfn_waveR, "rb");	// read from a binary file
	if (pfWave == NULL) {
		printf("Can't open the wf file %s!\n", psfn_waveR);
		return -10;
	}

	// 得到文件长度，。。。
	fseek(pfWave, 0, SEEK_END);	// 指针移到文件尾
	long llen_data;
	llen_data = ftell(pfWave);
	fseek(pfWave, 0, SEEK_SET);	// 指针重回文件头
	//

	fread(&m_header, sizeof(waveheader_t), 1, pfWave);

	if (strncmp(m_header.root_chunk_id, "RIFF", 4) != 0 || strncmp(m_header.riff_type_id, "WAVE", 4) != 0) {
		// not a wave file
		fclose(pfWave);
		printf("Not a wave file, abort !\n");
		return -20;
	}

	if (m_header.fmt_chunk_data_size > 16) {
		fseek(pfWave, m_header.fmt_chunk_data_size - 16, SEEK_CUR);
	}

	waveheader_ext_t header_ext;	// 扩展文件头
	fread(&header_ext, sizeof(waveheader_ext_t), 1, pfWave);
	long lLenWaveHeader = sizeof(waveheader_t) + m_header.fmt_chunk_data_size - 16 + sizeof(waveheader_ext_t);

	////////////////////////////////////////////////////////////////////////////////////////////////////

	llen_data -= lLenWaveHeader;

	//	printf("\nwave header length : %u\n", lLenWaveHeader);
	//	printf("wave data length : %u\n\n", llen_data);

	//	char			root_chunk_id[4];		// 'RIFF'
	//	unsigned long	root_chunk_data_size;	// length of root chunk
	//	printf("root_chunk_data_size : %u\n", m_header.root_chunk_data_size);
	//	char			riff_type_id[4];		// 'WAVE'
	//	char			fmt_chunk_id[4];		// 'fmt '
	//	unsigned long	fmt_chunk_data_size;	// length of sub_chunk, always 16 bytes
	//	printf("fmt_chunk_data_size(16) : %u\n", m_header.fmt_chunk_data_size);
	//	unsigned short	compression_code;		// always 1 = PCM-Code
	//	printf("compression_code(1) : %d\n", m_header.compression_code);

	//	unsigned short	num_of_channels;		// 1 = Mono, 2 = Stereo
	//	printf("num_of_channels(1 = Mono, 2 = Stereo) : %d\n", m_header.num_of_channels);

	//	unsigned long	sample_rate;			// Sample rate
	//	printf("sample_rate : %u\n", m_header.sample_rate);

	//	unsigned long	byte_p_sec;				// average bytes per sec
	//	printf("byte_p_sec(average bytes per sec) : %u\n", m_header.byte_p_sec);

	//	unsigned short	byte_p_sample;			// Bytes per sample, including the sample's data for all channels!
	//	printf("byte_p_sample : %d\n", m_header.byte_p_sample);
	//	unsigned short	bit_p_sample;			// bits per sample, 8, 12, 16
	//	printf("bit_p_sample(8, 12, or 16) : %d\n", m_header.bit_p_sample);

	//	char			data_chunk_id[4];		// 'data'
	//	unsigned long	data_chunk_size;		// length of data
	//	printf("data_chunk_size : %u\n", header_ext.data_chunk_size);
	if ((unsigned long)llen_data != header_ext.data_chunk_size) {
		//printf("llen_data = %u, not equal to data_chunk_size !\n", (unsigned long)llen_data);
		//		assert(0);
	}

	if (m_header.compression_code != 1) {
		fclose(pfWave);
		printf("Not in the reqired compression code, abort !\n");
		return -30;
	}
	if (m_header.bit_p_sample != 16) {
		fclose(pfWave);
		printf("Bits per sample is not 16, abort !\n");
		return -40;
	}
	if (m_header.sample_rate < m_unifiedSamplingRate) {
		fclose(pfWave);
		printf("Sampling rate is less than required, abort !\n");
		return -50;
	}
	if (m_header.num_of_channels > 2) {
		fclose(pfWave);
		printf("More than 2 channels, abort !\n");
		return -60;
	}

	// number of original samples in the audio file !!!
	//	m_lNumSamplesInFile = m_header_ext.data_chunk_size/m_header.byte_p_sample;
	// number of selected samples
	//	m_lNumSamplesInFile /= DOWN_SAMPLING_RATE;

	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	m_pfWaveR = pfWave;
	fseek(m_pfWaveR, lLenWaveHeader, SEEK_SET);	// 将文件指针移到 Waveform 数据开始处

	return 0;	// OK
}

// 按统一采样率做（欠）采样，并将多声道（多个采样值）合并成一个声道（一个采样值）
int WaveProcessor::MakeTargetSamplesData()
{
	assert(m_header.bit_p_sample == 16 || m_header.bit_p_sample == 8);
	// "m_header.sample_rate" : 原始采样频率
	// 原始采样率必须高于或等于 "m_unifiedSamplingRate"（此即目标采样率！！！）
	if (m_header.sample_rate < m_unifiedSamplingRate) {
		printf("m_header.sample_rate(%u) < %d !\n", m_header.sample_rate, m_unifiedSamplingRate);
		return -1;
	}

	//short *m_newlyMadeSamples = NULL;
	unsigned char *szOrginalSampsBuffer = NULL;
	// buffer to hold newly made down-sampling samples, 16 bits per sample!
	/* in order to accelerate
	m_newlyMadeSamples = (short *)malloc(sizeof(short)*BUFFER_SIZE_IN_SAMPLES_DS+
	(m_header.byte_p_sample)*BUFFER_SIZE_IN_SAMPLES_DS);
	*/

	if (m_newlyMadeSamples == NULL) {
		printf("No memory !\n");
		return -2;
	}
	szOrginalSampsBuffer = (unsigned char *)(m_newlyMadeSamples + BUFFER_SIZE_IN_SAMPLES_DS);

	// ////////////////////////////////////////////////////////////////////////////////////////

#ifdef __NORMALIZE_AUDIO_SAMPLES
	maxAbs = 0;
#endif

	int xxx;	// OK

	// 注意：欠采样信号和原采样信号对应的时长是一样的！！！

	// “原采样缓存”中第一个位置所存的采样的全局下标
	long iOriginalSampStartG = 0;
	// 本批生成的第一个欠采样点（即“欠采样缓存”中第一个点）的全局下标
	long iTargetSampStartG = 0;
	// total number of down-sampling samples made 
	unsigned long nNumNewlyMadeSampsTotal = 0;

	// 用完整信号，。。。
	int& sr = xxx;
	sr = m_unifiedSamplingRate;
	//_write(fhw, &sr, sizeof(int));	// 采样率
	//_write(fhw, &iTargetSampStartG, sizeof(unsigned long));	// 起始采样下标
	//_write(fhw, &nNumNewlyMadeSampsTotal, sizeof(unsigned long));	// 目标采样个数

	xxx = 0;
	//samplesVector.clear();
	memset(samplesArray, 0, sizeof(short)* SamplesVectorSize);
	//如果是多声道，取各个省道的平均
	if (m_header.num_of_channels > 1)
	{
		if (m_header.bit_p_sample == 16)
		{
			// 每从原始采样数据文件读“一批”原采样数据，就生成一批欠采样。
			while (!feof(m_pfWaveR)) {
				int nNumNewlyMadeSamps = 0;	// 本批（即从本次读取的原始信号数据）生成的欠采样点个数
				//（一）从当前歌曲音频数据文件读一块数据，。。。
				size_t uNumOrigSampsRead;
				uNumOrigSampsRead = fread(szOrginalSampsBuffer,
					m_header.byte_p_sample,	// 一个“采样”包含了一个采样时间点所有声道的数据
					BUFFER_SIZE_IN_SAMPLES_DS,	// 要读取的采样个数
					m_pfWaveR);
				if (uNumOrigSampsRead == 0) {// no content read
					break;
				}

				if (iTargetSampStartG == 0) {
					// 无论如何做欠采样，第一个原始采样总是要直接拷贝的。也就是说，欠采样信号的第一个样本点就是原信号的第一个
					// 样本点！
					assert(nNumNewlyMadeSamps == 0);
					// 第一个采样值照搬
					// 取各个声道的平均
					long lval;	// 用 "long" 保证计算过程不溢出
					lval = *((short *)szOrginalSampsBuffer);
					lval += *(((short *)szOrginalSampsBuffer) + 1);
					lval /= 2;
					m_newlyMadeSamples[nNumNewlyMadeSamps] = (short)lval;
					nNumNewlyMadeSamps++;
				}

				//（二）生成对应的新采样，。。。
				while (1) {
					// 核心是将欠采样点对应到某个原采样点（只有左采样点），或两个相邻的原采样点（即左右两个采样点）之间！！！

					// 与待求的欠采样点对应的原始采样点的全局下标（理论上不一定是整数）
					long iOriginalSampG;
					long iTargetSampG;
					// 根据欠采样点下标，。。。
					iTargetSampG = iTargetSampStartG + nNumNewlyMadeSamps;
					// 计算其对应的原采样点下标
					iOriginalSampG = (long)((double)iTargetSampG*m_header.sample_rate / m_unifiedSamplingRate + 0.5);
					if (iOriginalSampG > iOriginalSampStartG + uNumOrigSampsRead - 1) {
						// 还未读到“目标原采样点”，。。。
						iOriginalSampStartG += (long)uNumOrigSampsRead;
						break;
					}
					// 目标原采样点在缓存中的位置
					const unsigned char *puchar = szOrginalSampsBuffer +
						(m_header.byte_p_sample)*(iOriginalSampG - iOriginalSampStartG);
					// clrc_002
					//#ifdef __COMBINE_L_R_CHANNELS
					// 取各个声道的平均
					long lval = 0;
					long total = 0;
					//如果不够15个采样点，就取之前采样点的平均
					if (iOriginalSampG - iOriginalSampStartG < 15)
					{
						int sampleNum = iOriginalSampG - iOriginalSampStartG;
						for (int i = sampleNum; i >= 0; i--)
						{
							lval = *(((short *)puchar) - i);	// 本采样的第一个声道
							lval += *(((short *)puchar) - i + 1);	// 本采样的第二个声道
							lval /= 2;
							total += lval;
						}
						m_newlyMadeSamples[nNumNewlyMadeSamps] = (short)(total / (sampleNum + 1));

					}
					//否则，利用FIR滤波器使用16个采样点
					else
					{
						for (int i = 15; i >= 0; i--)
						{
							lval = *(((short *)puchar) - i);	// 本采样的第一个声道
							lval += *(((short *)puchar) - i + 1);	// 本采样的第二个声道
							lval /= 2;
							total += (long)(lval * coefficient[i]);
						}
						m_newlyMadeSamples[nNumNewlyMadeSamps] = (short)total;
					}
					nNumNewlyMadeSamps++;	// 本批欠采样个数计数

				}	// end of "while (1) {"
				for (int i = 0; i < nNumNewlyMadeSamps; i++)
					samplesArray[nNumNewlyMadeSampsTotal + i] = m_newlyMadeSamples[i];
				//（四）准备从音乐文件读入下一批原采样数据，。。。
				iTargetSampStartG += nNumNewlyMadeSamps;
				nNumNewlyMadeSampsTotal += nNumNewlyMadeSamps;
			}	// end of "while ( !feof(m_pfWaveR) ) {"
		}
		else if (m_header.bit_p_sample == 8)
		{
			// 每从原始采样数据文件读“一批”原采样数据，就生成一批欠采样。
			while (!feof(m_pfWaveR)) {
				int nNumNewlyMadeSamps = 0;	// 本批（即从本次读取的原始信号数据）生成的欠采样点个数
				//（一）从当前歌曲音频数据文件读一块数据，。。。
				size_t uNumOrigSampsRead;
				uNumOrigSampsRead = fread(szOrginalSampsBuffer,
					m_header.byte_p_sample,	// 一个“采样”包含了一个采样时间点所有声道的数据
					BUFFER_SIZE_IN_SAMPLES_DS,	// 要读取的采样个数
					m_pfWaveR);
				if (uNumOrigSampsRead == 0) {// no content read
					break;
				}

				if (iTargetSampStartG == 0) {
					// 无论如何做欠采样，第一个原始采样总是要直接拷贝的。也就是说，欠采样信号的第一个样本点就是原信号的第一个
					// 样本点！
					assert(nNumNewlyMadeSamps == 0);
					// 第一个采样值照搬
					// 取各个声道的平均
					long lval;	// 用 "long" 保证计算过程不溢出
					lval = *((char *)szOrginalSampsBuffer);
					lval += *(((char *)szOrginalSampsBuffer) + 1);
					lval *= 256;
					lval /= 2;
					m_newlyMadeSamples[nNumNewlyMadeSamps] = (short)lval;
					nNumNewlyMadeSamps++;
				}

				//（二）生成对应的新采样，。。。
				while (1) {
					// 核心是将欠采样点对应到某个原采样点（只有左采样点），或两个相邻的原采样点（即左右两个采样点）之间！！！

					// 与待求的欠采样点对应的原始采样点的全局下标（理论上不一定是整数）
					long iOriginalSampG;
					long iTargetSampG;
					// 根据欠采样点下标，。。。
					iTargetSampG = iTargetSampStartG + nNumNewlyMadeSamps;
					// 计算其对应的原采样点下标
					iOriginalSampG = (long)((double)iTargetSampG*m_header.sample_rate / m_unifiedSamplingRate + 0.5);
					if (iOriginalSampG > iOriginalSampStartG + uNumOrigSampsRead - 1) {
						// 还未读到“目标原采样点”，。。。
						iOriginalSampStartG += (long)uNumOrigSampsRead;
						break;
					}
					// 目标原采样点在缓存中的位置
					const unsigned char *puchar = szOrginalSampsBuffer +
						(m_header.byte_p_sample)*(iOriginalSampG - iOriginalSampStartG);
					// clrc_002
					//#ifdef __COMBINE_L_R_CHANNELS
					// 取各个声道的平均
					long lval = 0;
					long total = 0;
					//如果不够15个采样点，就取之前采样点的平均
					if (iOriginalSampG - iOriginalSampStartG < 15)
					{
						int sampleNum = iOriginalSampG - iOriginalSampStartG;
						for (int i = sampleNum; i >= 0; i--)
						{
							lval = *(((char *)puchar) - i);	// 本采样的第一个声道
							lval += *(((char *)puchar) - i + 1);	// 本采样的第二个声道
							lval *= 256;
							lval /= 2;
							total += lval;
						}
						m_newlyMadeSamples[nNumNewlyMadeSamps] = (short)(total / (sampleNum + 1));

					}
					//否则，利用FIR滤波器使用16个采样点
					else
					{
						for (int i = 15; i >= 0; i--)
						{

							lval = *(((char *)puchar) - i);	// 本采样的第一个声道
							lval += *(((char *)puchar) - i + 1);	// 本采样的第二个声道
							lval *= 256;
							lval /= 2;
							total += (long)(lval * coefficient[i]);
						}
						m_newlyMadeSamples[nNumNewlyMadeSamps] = (short)total;
					}
					nNumNewlyMadeSamps++;	// 本批欠采样个数计数

				}	// end of "while (1) {"
				for (int i = 0; i < nNumNewlyMadeSamps; i++)
					samplesArray[nNumNewlyMadeSampsTotal + i] = m_newlyMadeSamples[i];
				//（四）准备从音乐文件读入下一批原采样数据，。。。
				iTargetSampStartG += nNumNewlyMadeSamps;
				nNumNewlyMadeSampsTotal += nNumNewlyMadeSamps;
			}	// end of "while ( !feof(m_pfWaveR) ) {"
		}

	}


	//单声道的话，直接就是
	else
	{
		// 每从原始采样数据文件读“一批”原采样数据，就生成一批欠采样。
		while (!feof(m_pfWaveR)) {
			int nNumNewlyMadeSamps = 0;	// 本批（即从本次读取的原始信号数据）生成的欠采样点个数
			//（一）从当前歌曲音频数据文件读一块数据，。。。
			size_t uNumOrigSampsRead;
			uNumOrigSampsRead = fread(szOrginalSampsBuffer,
				m_header.byte_p_sample,	// 一个“采样”包含了一个采样时间点所有声道的数据
				BUFFER_SIZE_IN_SAMPLES_DS,	// 要读取的采样个数
				m_pfWaveR);
			if (uNumOrigSampsRead == 0) {// no content read
				break;
			}

			if (iTargetSampStartG == 0) {
				// 无论如何做欠采样，第一个原始采样总是要直接拷贝的。也就是说，欠采样信号的第一个样本点就是原信号的第一个
				// 样本点！
				assert(nNumNewlyMadeSamps == 0);
				// 第一个采样值照搬
				if (m_header.bit_p_sample == 16) {
					m_newlyMadeSamples[nNumNewlyMadeSamps] = *((short *)szOrginalSampsBuffer);
				}
				else if (m_header.bit_p_sample == 8) {
					m_newlyMadeSamples[nNumNewlyMadeSamps] = *((char *)szOrginalSampsBuffer);
					m_newlyMadeSamples[nNumNewlyMadeSamps] *= 256;
				}
				nNumNewlyMadeSamps++;
			}

			//（二）生成对应的新采样，。。。
			while (1) {
				// 核心是将欠采样点对应到某个原采样点（只有左采样点），或两个相邻的原采样点（即左右两个采样点）之间！！！

				// 与待求的欠采样点对应的原始采样点的全局下标（理论上不一定是整数）
				long iOriginalSampG;
				long iTargetSampG;
				// 根据欠采样点下标，。。。
				iTargetSampG = iTargetSampStartG + nNumNewlyMadeSamps;
				// 计算其对应的原采样点下标
				iOriginalSampG = (long)((double)iTargetSampG*m_header.sample_rate / m_unifiedSamplingRate + 0.5);
				if (iOriginalSampG > iOriginalSampStartG + uNumOrigSampsRead - 1) {
					// 还未读到“目标原采样点”，。。。
					iOriginalSampStartG += (long)uNumOrigSampsRead;
					break;
				}
				// 目标原采样点在缓存中的位置
				const unsigned char *puchar = szOrginalSampsBuffer +
					(m_header.byte_p_sample)*(iOriginalSampG - iOriginalSampStartG);
				long lval = 0;
				long total = 0;
				//如果不够15个采样点，就取之前采样点的平均
				if (iOriginalSampG - iOriginalSampStartG < 15)
				{
					int sampleNum = iOriginalSampG - iOriginalSampStartG;
					for (int i = sampleNum; i >= 0; i--)
					{
						if (m_header.bit_p_sample == 16) {
							lval = *(((short *)puchar) - i);
						}
						else if (m_header.bit_p_sample == 8) {
							lval = *(((char *)puchar) - i);
							lval *= 256;
						}
						total += lval;
					}
					m_newlyMadeSamples[nNumNewlyMadeSamps] = (short)(total / (sampleNum + 1));

				}
				//否则，利用FIR滤波器使用16个采样点
				else
				{
					for (int i = 15; i >= 0; i--)
					{

						if (m_header.bit_p_sample == 16) {
							lval = *(((short *)puchar) - i);
						}
						else if (m_header.bit_p_sample == 8) {
							lval = *(((char *)puchar) - i);
							lval *= 256;
						}
						total += long(lval * coefficient[i]);
					}
					m_newlyMadeSamples[nNumNewlyMadeSamps] = (short)total;
				}
				nNumNewlyMadeSamps++;	// 本批欠采样个数计数

			}	// end of "while (1) {"
			for (int i = 0; i < nNumNewlyMadeSamps; i++)
				samplesArray[nNumNewlyMadeSampsTotal + i] = m_newlyMadeSamples[i];
			//（四）准备从音乐文件读入下一批原采样数据，。。。
			iTargetSampStartG += nNumNewlyMadeSamps;
			nNumNewlyMadeSampsTotal += nNumNewlyMadeSamps;
		}	// end of "while ( !feof(m_pfWaveR) ) {"
	}
	m_newlyMadeSamplesNumber = nNumNewlyMadeSampsTotal;
	return xxx;	// OK
}


void WaveProcessor::GetSamplesVector(short* all_time_data, unsigned long& all_time_data_size)
{
	for (unsigned int i = 0; i < m_newlyMadeSamplesNumber; i++)
		all_time_data[i] = samplesArray[i];
	//all_time_data = samplesVector;
	all_time_data_size = m_newlyMadeSamplesNumber;
}
