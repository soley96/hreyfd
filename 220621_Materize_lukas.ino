// GENERAL BASE CODE FOR HREYFÐ


#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>


// Audio devices
AudioControlSGTL5000     audioShield;    
AudioInputI2S            i2s1;         
AudioOutputI2S           i2s2;        
AudioAmplifier           amp1;
AudioEffectDelay         delay1;
AudioFilterBiquad        filter;
AudioMixer4              mixer1;
AudioEffectBitcrusher    left_BitCrusher;
AudioEffectBitcrusher    right_BitCrusher;


// Patch Bay
AudioConnection          c1(i2s1, 0, delay1, 0);
AudioConnection          c2(delay1, 0, mixer1, 0);
AudioConnection          c3(delay1, 1, mixer1, 1);
AudioConnection          c4(mixer1, 0, filter, 0);
AudioConnection          c5(filter, 0, amp1, 0);
AudioConnection          c6(amp1, 0, left_BitCrusher, 0);
AudioConnection          c7(amp1, 0, right_BitCrusher, 0);
AudioConnection          c8(left_BitCrusher, 0, i2s2, 0);
AudioConnection          c9(right_BitCrusher, 0, i2s2, 1);


// Pin Outs
const int mic = AUDIO_INPUT_MIC; // er hægt að hækka í mæknum? 
const int led = 3;

// settings
float vol = 0.8;
float volMin = 0.0;
float volMax = 0.7;


// variables
int delayTime1 = 25;
int delayTime2 = 25;


// gyro set up
#define    MPU9250_ADDRESS            0x68

#define    ACC_FULL_SCALE_2_G        0x00  
#define    ACC_FULL_SCALE_4_G        0x08
#define    ACC_FULL_SCALE_8_G        0x10
#define    ACC_FULL_SCALE_16_G       0x18

const int numReadings = 20;

int axReadings[numReadings];      
int axReadIndex = 0;              
float axTotal = 0;                 

int ayReadings[numReadings];      
int ayReadIndex = 0;              
int ayTotal = 0;                 

int azReadings[numReadings];      
int azReadIndex = 0;              
int azTotal = 0;                 

float axAverage = 0;               
float ayAverage = 0;               
float azAverage = 0;  

//BitCrusher
int current_CrushBits = 16; //this defaults to passthrough.
int current_SampleRate = 44100; // this defaults to passthrough.

// boolean compressorOn = true; // default this to off.


void setup() {

  AudioMemory(300); // this needs to be pretty high for the delays to work
  Serial.begin(9600);
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH); 

  audioShield.enable();
  audioShield.audioPreProcessorEnable();
  audioShield.surroundSoundEnable();  
  audioShield.volume(0.3);
  audioShield.inputSelect(mic);
  audioShield.micGain(40);
  audioShield.lineOutLevel(13);
  
  // autoVolume control / simple compressor
  audioShield.autoVolumeEnable(); 
  audioShield.autoVolumeControl(0, 1, 0.9, -36, 0.2, 10);  // maxGain, response, hardLimit, threshold, attack, decay


  // gyro
  Wire.begin();
  I2CwriteByte(MPU9250_ADDRESS,29,0x06);
  // Configure accelerometers range
  I2CwriteByte(MPU9250_ADDRESS,28,ACC_FULL_SCALE_4_G);
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    axReadings[thisReading] = 0;
  }
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    ayReadings[thisReading] = 0;
  }
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    azReadings[thisReading] = 0;
  }

  
}


// gyro 
// This function read Nbytes bytes from I2C device at address Address. 
// Put read bytes starting at register Register in the Data array. 

void I2Cread(uint8_t Address, uint8_t Register, uint8_t Nbytes, uint8_t* Data)  
// is this possibly something that needs to be changed in order to address the "other" SDA and SCL? 

{
  // Set register address
  Wire.beginTransmission(Address);
  Wire.write(Register);
  Wire.endTransmission();
  
  // Read Nbytes
  Wire.requestFrom(Address, Nbytes); 
  uint8_t index=0;
  while (Wire.available())
    Data[index++]=Wire.read();
}


// gyro
// Write a byte (Data) in device (Address) at register (Register)
void I2CwriteByte(uint8_t Address, uint8_t Register, uint8_t Data)
{
  // Set register address
  Wire.beginTransmission(Address);
  Wire.write(Register);
  Wire.write(Data);
  Wire.endTransmission();

  // synth
/*   synth.frequency(100);
  int current_waveform = WAVEFORM_SQUARE;  
  synth.begin(current_waveform);
  */

  // filter

  filter.setLowpass(0, 10000, 0.6);
  filter.setHighpass(0, 150, 0.8);
  filter.setLowShelf(0, 200, -6, 0.4);
  filter.setHighShelf(0, 500, -6, 0.2);  // stage, frequency, gain, slope
//  filter.setHighShelf(0, 2000, -6, 0.4); 
 
}



void loop() {

  delay1.delay(0, delayTime1);
  delay1.delay(1, delayTime2);  
  amp1.gain(vol);

  
  // Bitcrusher
  left_BitCrusher.bits(current_CrushBits); //set the crusher to defaults. This will passthrough clean at 16,44100
  left_BitCrusher.sampleRate(current_SampleRate); //set the crusher to defaults. This will passthrough clean at 16,44100
  right_BitCrusher.bits(current_CrushBits); //set the crusher to defaults. This will passthrough clean at 16,44100
  right_BitCrusher.sampleRate(current_SampleRate); //set the crusher to defaults. This will passthrough clean at 16,44100
  //Bitcrusher


// gyro
  // Read accelerometer
  uint8_t Buf[14];
  I2Cread(MPU9250_ADDRESS,0x3B,14,Buf);
  
  // Create 16 bits values from 8 bits data
  int16_t ax=-(Buf[0]<<8 | Buf[1]);
  int16_t ay=-(Buf[2]<<8 | Buf[3]);
  int16_t az=Buf[4]<<8 | Buf[5];

// mapping the range
  ax = map(ax, -10000, 10000,-100, 100);
  ay = map(ay, -10000, 10000,-100, 100);
  az = map(az, -10000, 10000,-100, 100);
  
  ax = constrain(ax, -100, 100);
  ay = constrain(ay, -100, 100);
  az = constrain(az, -100, 100);  

// ax smooth
  axTotal = axTotal - axReadings[axReadIndex];
  axReadings[axReadIndex] = ax;
  axTotal = axTotal + axReadings[axReadIndex];
  axReadIndex = axReadIndex + 1;
  if (axReadIndex >= numReadings) {
    axReadIndex = 0;
  }
  axAverage = axTotal / numReadings;

// ay smooth
  ayTotal = ayTotal - ayReadings[ayReadIndex];
  ayReadings[ayReadIndex] = ay;
  ayTotal = ayTotal + ayReadings[ayReadIndex];
  ayReadIndex = ayReadIndex + 1;
  if (ayReadIndex >= numReadings) {
    ayReadIndex = 0;
  }
  ayAverage = ayTotal / numReadings;

// az smooth
  azTotal = azTotal - azReadings[azReadIndex];
  azReadings[azReadIndex] = az;
  azTotal = azTotal + azReadings[azReadIndex];
  azReadIndex = azReadIndex + 1;
  if (azReadIndex >= numReadings) {
    azReadIndex = 0;
  }
  azAverage = azTotal / numReadings;


// mapping
  axAverage = constrain(axAverage, -75, 90);
  axAverage = map(axAverage, -75, 90, volMin, volMax);
  
//  ayAverage = constrain(ayAverage, 10, 100);
//  rTime = map(ayAverage, 10, 100, reverbMin, reverbMax); 
//  rTime = map(ayAverage, 10, 100, 0, 1);
//  ayAverage = map(ayAverage, 10, 100, 0, 1);


  azAverage = map(azAverage, -100, 100, 0, 1);
  azAverage = constrain(azAverage, 0.0, 1.0);
  
  vol = axAverage;
//  reverb.reverbTime(rTime);
//  reverb.roomsize(0.25);
//  reverb.damping(rTime);
  current_CrushBits = map(ayAverage, 10, 100, 16, 1);

  
  // Display values
  
//  Serial.print ("ax: ");
  Serial.print (axAverage, DEC); 
  Serial.print ("\t");
//  Serial.print ("ay: ");
  Serial.print (ayAverage,DEC);
  Serial.print ("\t");
//  Serial.print ("az: "); 
  Serial.print (azAverage,DEC);  
  Serial.print ("\t");  
  
//  Serial.println("");
  delay(5);      
 
  Serial.print("vol: ");
  Serial.print(vol);
  Serial.print ("\t");
  Serial.print("crushBits: ");
  Serial.print(current_CrushBits); 
//  Serial.print(", delayTime");
//  Serial.print(delayTime);
  Serial.println(", ");

}
