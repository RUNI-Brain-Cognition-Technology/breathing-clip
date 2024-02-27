/*  Proof-of-concept code for Reichmann University Respiration Sensor
 *  Shreyas Renganathan, 9/27/2023
 *  This sketch is written for the M5StickCPlus.
 *  This sketch continuously reads the onboard IMU (MPU6886), bandpass-filters the data, and streams it over a WiFi network via UDP broadcast
 *  Output data rate is ~400Hz in empirical testing.
 *  New signal processing algorithm maps the output breathing signal b(t) to -1000<b(t)<1000
 *  This is achieved by bandpass filtering the signal to remove noise, then clipping it to a preset range narrower than the observed range
 *  This clipped sinewave is then lowpass filtered again to reconstruct a sinusoidal signal. 
 *  Notes:
 *  - In /utility/MPU6886.h, set maximum resolution by editing line 66 to: Ascale Acscale = AFS_2G;
 */
#include <M5StickCPlus.h>
#include "WiFi.h"
#include "AsyncUDP.h"

const int LED_PIN=10;
const bool OFF=HIGH;
const bool ON=LOW;
bool STREAM_EN = false;
bool LED_STATE = ON;
int streamPrint = false;

/* Enter patient ID# here */
const int patient_id = 2308;

/* Set WiFi credentials here */
const char* ssid = "SJMnetwork2.4";
const char* password = "Muddy0607";

/* Enter device IP address and port here*/
//IP address doesn't really matter, it's just a placeholder to start the connection
//Gets replaced by a dynamic IP after connection
const int ipAddress[4] = {192,168,1,100}; 
//Port matters, ensure this matches the port set on your receiver
const int port = 1234;

float accX, accY, accZ;
//float gyrX, gyrY, gyrZ;

float filt_sig = 0.0;
float dc_sig = 0.0;
float ac_sig = 0.0;
float upper_limit=0.2;
float lower_limit=-0.2;
float ac_sig_reconst=0.0;
int ac_sig_reconst_1000=0;

unsigned long elapsedTime=millis();
unsigned long startTime=millis();

// Class to define lowpass filter.
// Order can be 1 (first order) or 2 (second order) */
template <int order>
class LowPass{
  private:
    float a[order];
    float b[order+1];
    float omega0;
    float dt;
    bool adapt;
    float tn1 = 0;
    float x[order+1]; // Raw values
    float y[order+1]; // Filtered values

  public:  
    LowPass(float f0, float fs, bool adaptive){
      // f0: cutoff frequency (Hz)
      // fs: sample frequency (Hz)
      // adaptive: if 1, sample frequency will auto-adjust based on time history

      // Convert cutoff frequency (Hz) to angular frequency (rad/s)
      // i.e. w0=2*pi*f0
      omega0 = 6.28318530718*f0;
      
      //Calculate timestep from sampling frequency
      dt = 1.0/fs;
      
      adapt = adaptive;
      tn1 = -dt;
      for(int k = 0; k < order+1; k++){
        x[k] = 0;
        y[k] = 0;        
      }
      // Set filter coefficients
      setCoef();
    }
    
    // Set filter coefficients
    void setCoef(){
      if(adapt){
        float t = micros()/1.0e6;
        dt = t - tn1;
        tn1 = t;
      }
      
      float alpha = omega0*dt;
      if(order==1){
        a[0] = -(alpha - 2.0)/(alpha+2.0);
        b[0] = alpha/(alpha+2.0);
        b[1] = alpha/(alpha+2.0);        
      }
      if(order==2){
        float alphaSq = alpha*alpha;
        float beta[] = {1, sqrt(2), 1};
        float D = alphaSq*beta[0] + 2*alpha*beta[1] + 4*beta[2];
        b[0] = alphaSq/D;
        b[1] = 2*b[0];
        b[2] = b[0];
        a[0] = -(2*alphaSq*beta[0] - 8*beta[2])/D;
        a[1] = -(beta[0]*alphaSq - 2*beta[1]*alpha + 4*beta[2])/D;      
      }
    }

    // For given raw value x, calculate and output filtered value y
    float filt(float xn){
      if(adapt){
        //if adaptive=1, update filter coefficients each time this function is called
        setCoef();      
      }
      y[0] = 0;
      x[0] = xn;
      
      // Calculate filtered values
      for(int k = 0; k < order; k++){
        y[0] += a[k]*y[k+1] + b[k]*x[k];
      }
      y[0] += b[order]*x[order];

      // Save history of raw and filtered values
      for(int k = order; k > 0; k--){
        y[k] = y[k-1];
        x[k] = x[k-1];
      }
  
      // Output filtered value    
      return y[0];
    }
};

/*Instantiate lowpass filters
 * f0: cutoff frequency (Hz)
 * fs: sample frequency (Hz)
 * adaptive: boolean flag, if set to 1, the code will automatically set the sample frequency based on the time history.
 */
// Lowpass filter on raw data. Cutoff freq of 0.9Hz comes from Hughes et al (2020)
// https://www.ncbi.nlm.nih.gov/pmc/articles/PMC7363979/
LowPass<2> raw_lp(0.9,1000,true);

// Lowpass filter to extract DC offset i.e low-frequency noise like motion artifacts
// Subtracting this DC offset from the main lowpass filter above effectively bandpass-filters the signal
// Cutoff freq of 0.15Hz was set observationally- trying to balance rapid tracking of the DC offset with capturing breath signals below the lower frequency bound of normal (12bpm=0.2Hz)
LowPass<2> dc_lp(0.15,1000,true);

// Lowpass filter to reconstruct sine wave from clipped breathing signal
LowPass<2> rc_lp(0.7,1000,true);

//Instantiate UDP
AsyncUDP udp;
      
void setup() {
  pinMode(LED_PIN,OUTPUT);
  digitalWrite(LED_PIN,ON);
  //Initialize M5StickCPlus for IMU reads and button reads
  //Disable LCD screen
  M5.begin(0,1,1);
  //Turn off LCD backlight
  M5.Axp.SetLDO2(false);
  //Initialize IMU
  M5.IMU.Init();
  //Initialize Serial communication (necessary only for debug)
  Serial.begin(115200);
  //Connect to an existing WiFI network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("WiFi Failed");
      while(1) {
          delay(1000);
      }
  }
  if(udp.connect(IPAddress(ipAddress[0],ipAddress[1],ipAddress[2],ipAddress[3]), port)) {
      Serial.println("UDP connected");
  }
  //Need to have this so M5Stack doesn't complain about LED not being initialized
  ledcSetup(0,0,12);
}
void loop() {
  //Check for new button presses using M5Stack
  M5.update();
  //If main button was pressed, toggle stream enable and reset timer;
  if(M5.BtnA.wasPressed()){
    STREAM_EN=!STREAM_EN;
    startTime=millis();
  }
  //Update elapsed time
  elapsedTime=millis()-startTime;

  //Get raw accelerometer data
  M5.IMU.getAccelData(&accX, &accY, &accZ);
  
  //Lowpass-filter the raw data
  filt_sig = raw_lp.filt(accZ)*100.0;
  //Lowpass-filter at a lower freq to extract DC offset
  dc_sig = dc_lp.filt(accZ)*100.0;
  //Subtract DC offset from data to get signal centered at zero
  ac_sig = filt_sig - dc_sig;

  //Clip signal on both top and bottom
  if (ac_sig>upper_limit) ac_sig=upper_limit;
  if (ac_sig<lower_limit) ac_sig=lower_limit;

  //Reconstruct sinewave by lowpass-filtering the clipped signal
  ac_sig_reconst=rc_lp.filt(ac_sig)*1000;
  
  //Clip again to remove filter artifacts
  if (ac_sig_reconst>upper_limit*1000.0) ac_sig_reconst = upper_limit*1000.0;
  if (ac_sig_reconst<lower_limit*1000.0) ac_sig_reconst = lower_limit*1000.0;

  //Rescale reconstructed signal to -1000<signal<1000
  //Scaling factor is 1000/(upper_limit*10000)=1/upper_limit
  ac_sig_reconst_1000=ac_sig_reconst*(1.0/upper_limit);

  //Send broadcast on port 1234
  char buf[20];
  if(STREAM_EN){
    LED_STATE=ON;
    //Construct buffer to send over UDP
    sprintf(buf,"%d,%ul,%d",patient_id,elapsedTime,ac_sig_reconst_1000);
    udp.broadcastTo(buf, 1234);
    if (streamPrint){
      Serial.print(ac_sig_reconst_1000);
      Serial.println();
    }
  }
  else LED_STATE=OFF;
  digitalWrite(LED_PIN,LED_STATE);
  delay(2);
}

