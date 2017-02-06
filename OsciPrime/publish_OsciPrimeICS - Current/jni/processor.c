
    /**
    OsciPrime an Open Source Android Oscilloscope
    Copyright (C) 2012  Manuel Di Cerbo, Nexus-Computing GmbH Switzerland
    Copyright (C) 2012  Andreas Rudolf, Nexus-Computing GmbH Switzerland

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#include <jni.h>
#include <math.h>
#include <stdlib.h>
#include <android/log.h>
#include <time.h>
#include <stdio.h>

#define RISING_EDGE 0
#define MODE_SINGLESHOT 1
#define CH1 0
#define CH2 1

#define  LOG_TAG    ">==<libprocessor>==<"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)

static double now_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000. + tv.tv_usec/1000.;
}


static int trigger(signed int* buffer, unsigned int length, signed int trigger, char edge);
static void selectPoints(signed int* buffer, int buflen, signed int* dest, int destlen, unsigned int triggerIndex, jint nthVal);
static void selectPoints2(unsigned char* buffer, int buflen, signed int* dest, int destlen, unsigned int triggerIndex, jint nthVal,jfloat caloffset, char channel);

/**
	Return that a trigger has been found
**/
jint
Java_ch_nexuscomputing_android_osciprimeics_source_OsciUsbSource_process(
	JNIEnv * env,
	jclass clazz,
	jobject buf,
	jobject copy,
	jint buflen,
	jintArray interleavedCh1,
	jintArray interleavedCh2,
	jintArray previewCh1,
	jintArray previewCh2,
	jint interleavelen,
	jint previewLen,
	jint interleave,
	jfloat calibrationOffsetCh1,
	jfloat calibrationOffsetCh2,
	jfloat attenuationCh1,
	jfloat attenuationCh2,
	jint triggerLevelCh1,
	jint triggerLevelCh2,
	jint triggerChannel,
	jint triggerEdgeCh1,
	jint triggerEdgeCh2,
	jint mode,
	jint oldTrigger,
	jfloatArray returnValues){

	unsigned char* buffer = (unsigned char*)(*env)->GetDirectBufferAddress(env,buf);
	unsigned char* buffercopy = (unsigned char*)(*env)->GetDirectBufferAddress(env,copy);

	//LOGD("%d", buffer[0]);
	
	memcpy(buffercopy, buffer, buflen);
	//signed int* ch1copy = (signed int*)(*env)->GetDirectBufferAddress(env,copyCh1);
	//signed int* ch2copy = (signed int*)(*env)->GetDirectBufferAddress(env,copyCh2);
	//signed int* ch1Interl = (signed int*)(*env)->GetDirectBufferAddress(env,interleavedCh1);
	//signed int* ch2Interl = (signed int*)(*env)->GetDirectBufferAddress(env,interleavedCh2);
	
	int i = 0, j = 0;
	int len = buflen/2;
	unsigned int tr = 0;
	signed int value = 0;
	static float retv[2];
	
	//space for split channels
	//signed int* ch1 = (signed int*)(malloc(len*sizeof(signed int)));
	//signed int* ch2 = (signed int*)(malloc(len*sizeof(signed int)));
	//signed int* ch1Interl = (signed int*)(malloc(interleavelen*sizeof(signed int)));
	//signed int* ch2Interl = (signed int*)(malloc(interleavelen*sizeof(signed int)));
	
	//signed int* ch1 = (signed int*)(*env)->GetPrimitiveArrayCritical(env,copyCh1,0);
	//signed int* ch2 = (signed int*)(*env)->GetPrimitiveArrayCritical(env,copyCh2,0);
	signed int* ch1Interl = (signed int*)(*env)->GetPrimitiveArrayCritical(env,interleavedCh1,0);
	signed int* ch2Interl = (signed int*)(*env)->GetPrimitiveArrayCritical(env,interleavedCh2,0);

	signed int* ch1Preview = (signed int*)(*env)->GetPrimitiveArrayCritical(env,previewCh1,0);
	signed int* ch2Preview = (signed int*)(*env)->GetPrimitiveArrayCritical(env,previewCh2,0);
/*
	//split channels
	for(i = 0; i < len; i++){
		ch1[i] = buffer[2*i];
		ch1[i] -= 128;
		ch1[i] *= -1;
		//ch1offset += ch1[i]/(len/2.0);
		ch1[i] -= calibrationOffsetCh1;
		
		ch2[i] = buffer[2*i+1];
		ch2[i] -= 128;
		ch2[i] *= -1;
		//ch2offset += ch2[i]/(len/2.0);
		ch2[i] -= calibrationOffsetCh2;
	}

*/
	int ref = len/2;//this is the reference, "best" expected trigger index
	signed int lastSample = 0;
	signed int lastSample2 = 0;
	signed int lastSample3 = 0;
	signed int lastSample4 = 0;
	signed int lastSample5 = 0;
	
	signed int n;
	unsigned long long bestCost = ((unsigned long long)len)*((unsigned long long)len);
	unsigned long long curCost = 0;
	int triggerIndex = -1;
	int edge = (triggerChannel == CH1 ? triggerEdgeCh1 : triggerEdgeCh2);
	int lvl = (triggerChannel == CH1 ? (signed int)(triggerLevelCh1/attenuationCh1) : (signed int)(triggerLevelCh2/attenuationCh2));

	double now = now_ms();

	signed int u,v = 0;
	signed long ch1off = 0,ch2off = 0;
	
	static unsigned char last_edge  = RISING_EDGE;
	
	if(oldTrigger < 0){//this means we are in "retrigger" mode
		if(edge == RISING_EDGE){//this is actually a falling edge, since we invert in the java view
			for(i = 0; i < len; i+= interleave){
				u = -buffer[2*i]+128-(int)calibrationOffsetCh1;
				v = -buffer[2*i+1]+128-(int)calibrationOffsetCh2;
				ch1off += u+(int)calibrationOffsetCh1;
				ch2off += v+(int)calibrationOffsetCh2;
				n = ((char)triggerChannel == CH1 ? u : v);//trigger
				if(n > lvl && lastSample <= lvl && lastSample2 <= lvl){
					if(mode == MODE_SINGLESHOT){//take first edge
						if(triggerIndex > 0)
							continue;//to calculate offset any
						triggerIndex = i;
					}
					curCost = abs(i-ref);
					if(curCost < bestCost){
						bestCost = curCost;
						triggerIndex = i;
					}
				}
				lastSample5 = lastSample4;
				lastSample4 = lastSample3;
				lastSample3 = lastSample2;
				lastSample2 = lastSample;
				lastSample = n;
			}
			if(edge != last_edge){
				LOGD("switched to code part in RISING_EDGE");
			}
		}else{
			for(i = 0; i < len; i+= interleave){//this is actually a rising edge, since we invert in the java view
				u = -buffer[2*i]+128-(int)calibrationOffsetCh1;
				v = -buffer[2*i+1]+128-(int)calibrationOffsetCh2;
				ch1off += u+(int)calibrationOffsetCh1;
				ch2off += v+(int)calibrationOffsetCh2;
				//lastSample5 > lvl  && lastSample4 > lvl && lastSample3 > lvl && lastSample2 > lvl &&
				n = ((char)triggerChannel == CH1 ? u : v);//trigger
				if(n < lvl &&  lastSample >= lvl && lastSample2 >= lvl){
					if(mode == MODE_SINGLESHOT){//take first edge
						if(triggerIndex > 0)
							continue;//to calculate offset any
						triggerIndex = i;
					}
					curCost = abs(i-ref);
					if(curCost < bestCost){
						bestCost = curCost;
						triggerIndex = i;
					}
				}
				lastSample5 = lastSample4;
				lastSample4 = lastSample3;
				lastSample3 = lastSample2;
				lastSample2 = lastSample;
				lastSample = n;
			}
			if(edge != last_edge){
				LOGD("switched to code part in FALLING_EDGE");
			}
		}	
		last_edge = edge;
		tr = triggerIndex;
	}else{//mens we don't want to trigger again
		tr = oldTrigger;
	}
	
	//generate preview
    if(previewLen != -1){//ch1Preview has double the capacity of previewLen
        int segementLength = len/previewLen;
        int i = 0; j = 0;
        int u = 0, v = 0;
        for(i = 0; i < previewLen; i++){
            int min1 = 256, max1 = 0;
            int min2 = 256, max2 = 0;
            for(j = 0; j < segementLength; j++){
                u = buffer[2*(i*segementLength+j)];
                v = buffer[2*(i*segementLength+j)+1];
                if(u > max1) max1 = u;
                if(u < min1) min1 = u;
                if(v > max2) max2 = v;
                if(v < min2) min2 = v;
            }
            ch1Preview[2*i] = min1;
            ch1Preview[2*i+1] = max1;
            ch2Preview[2*i] = min2;
            ch2Preview[2*i+1] = max2;
        }
    }

//	if(triggerChannel == CH1){
//		tr = trigger(ch1, len, (signed int)(triggerLevelCh1/attenuationCh1), (char)triggerEdgeCh1);
//	}else{
//		tr = trigger(ch2, len, (signed int)(triggerLevelCh2/attenuationCh2), (char)triggerEdgeCh2);
//	}
	
	selectPoints2(buffer, len, ch1Interl, interleavelen, tr, interleave,calibrationOffsetCh1, CH1);
	selectPoints2(buffer, len, ch2Interl, interleavelen, tr, interleave,calibrationOffsetCh2, CH2);
	//selectPoints(ch1, len, ch1Interl, interleavelen, tr, interleave);
	//selectPoints(ch2, len, ch2Interl, interleavelen, tr, interleave);
	
	retv[0] = (float)(ch1off/(double)(len/interleave));
	retv[1] = (float)(ch2off/(double)(len/interleave));
	
	
	//(*env)->SetIntArrayRegion(env, copyCh1, 0, len, ch1);
	//(*env)->SetIntArrayRegion(env, copyCh2, 0, len, ch2);
	//(*env)->SetIntArrayRegion(env, interleavedCh1, 0, interleavelen, ch1Interl);
	//(*env)->SetIntArrayRegion(env, interleavedCh2, 0, interleavelen, ch2Interl);
	
	//(*env)->ReleasePrimitiveArrayCritical(env,copyCh1,ch1,0);
	//(*env)->ReleasePrimitiveArrayCritical(env,copyCh2,ch2,0);
	(*env)->ReleasePrimitiveArrayCritical(env,interleavedCh1,ch1Interl,0);
	(*env)->ReleasePrimitiveArrayCritical(env,interleavedCh2,ch2Interl,0);
	(*env)->ReleasePrimitiveArrayCritical(env,previewCh1,ch1Preview,0);
	(*env)->ReleasePrimitiveArrayCritical(env,previewCh2,ch2Preview,0);
	
	(*env)->SetFloatArrayRegion(env, returnValues, 0, 2, &retv[0]);
	
	//LOGD("processing took %.1f [ms]",(now_ms()-now));
	//free(ch1Interl);
	//free(ch2Interl);
	//free(ch1);
	//free(ch2);

	return tr;
}

static int trigger(signed int* buffer, unsigned int length, signed int trigger, char edge){	
	
	int ref = length/2;//this is the reference, "best" expected trigger
	signed int lastSample = 0;
	signed int n;
	int i = 0, j = 0;
	unsigned long long bestCost = ((unsigned long long)length)*((unsigned long long)length);
	unsigned long long curCost = 0;
	int triggerIndex = -1;
	static int start = 0;

	if(edge == RISING_EDGE){
		for(i = 0; i < length; i++){
			n = buffer[i];
			if(n >= trigger && lastSample < trigger){
				curCost = abs(i-ref);
				if(curCost < bestCost){
					bestCost = curCost;
					triggerIndex = i;
				}
			}
			lastSample = n;
		}
		start = 1;
	}else{
		for(i = 0; i < length; i++){
			n = buffer[i];
			if(n <= trigger && lastSample > trigger){
				curCost = abs(i-ref);
				if(curCost < bestCost){
					bestCost = curCost;
					triggerIndex = i;
				}
			}
			lastSample = n;
		}
	}	

	return triggerIndex;
}

static void selectPoints2(unsigned char* buffer, int buflen, signed int* dest, int destlen, unsigned int triggerIndex, jint nthVal, jfloat caloffset, char channel){
	int i = 0, j = 0;
	int start = triggerIndex-destlen/2*nthVal;
	int stop = triggerIndex+destlen/2*nthVal;
	if(start < 0){
		start = 0;//no trigger found
	}
	if(stop > buflen-1){
		start = buflen-1-destlen*nthVal;//trigger to far away
		//maybe it should be -2 ...
		if(start < 0)
			start = 0;
	}
	//__android_log_print(ANDROID_LOG_ERROR,"selectPoints","start %d stop %d buflen %d destlen %d trigger %d interleave %d", start, stop, buflen, destlen, triggerIndex, nthVal);
	j = start;
	for(i = 0; i < destlen; i++){
		if(j > buflen-1){//so the interleave would overlap, that is not good
			dest[i] = 0;//this should actually never happen unless start is miss
						//calculated
			__android_log_print(ANDROID_LOG_ERROR,"selectPoints","setting value 0");
		}else{
			if(channel == CH1)
				dest[i] = -buffer[2*j]+128-caloffset;
			else
				dest[i] = -buffer[2*j+1]+128-caloffset;
		}
		j += nthVal;
	}
}

static void selectPoints(signed int* buffer, int buflen, signed int* dest, int destlen, unsigned int triggerIndex, jint nthVal){
	int i = 0, j = 0;
	int start = triggerIndex-destlen/2*nthVal;
	int stop = triggerIndex+destlen/2*nthVal;
	if(start < 0){
		start = 0;//no trigger found
	}
	if(stop > buflen-1){
		start = buflen-1-destlen*nthVal;//trigger to far away
		//maybe it should be -2 ...
		if(start < 0)
			start = 0;
	}
	//__android_log_print(ANDROID_LOG_ERROR,"selectPoints","start %d stop %d buflen %d destlen %d trigger %d interleave %d", start, stop, buflen, destlen, triggerIndex, nthVal);
	j = start;
	for(i = 0; i < destlen; i++){
		if(j > buflen-1){//so the interleave would overlap, that is not good
			dest[i] = 0;//this should actually never happen unless start is miss
						//calculated
			__android_log_print(ANDROID_LOG_ERROR,"selectPoints","setting value 0");
		}else{
			dest[i] = buffer[j];
		}
		j += nthVal;
	}
}
