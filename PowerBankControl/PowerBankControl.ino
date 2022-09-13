int pin[] = {1, 2, 4, 5, 6};
int servoPin[] = {16, 17};

const int low = 22*16;
const int high = 29*16;

int maxValues[] = {3889, 3847, 3491, 4016, 4095};

void pushButton(int channel)
{
  ledcWrite(channel, low);
  delay(600);
  ledcWrite(channel, high);  
  delay(600);
}

void setup() 
{
  pinMode(0, INPUT_PULLUP);
  Serial.begin(115200);  
  for(int i = 0; i < 2; i++)
  {
    ledcSetup(i, 50, 12);
    ledcAttachPin(servoPin[i], i);
    ledcWrite(i, high);  
  }
  delay(1000);
}

int getSample(int ch)
{
  return min(255, (analogRead(pin[ch]) * 255) / maxValues[ch]);
}

int scanMinMax(int timeout = 5000)
{
  const int sampleAvg = 128;
  const int sampleCount = 100;
  
  int minMaxBits = 0;
  unsigned long t = millis();
  while(t + timeout > millis())
  {
    int s[5];
    for(int j = 0; j < sampleAvg; j++)
      for(int i = 0; i < 5; i++)
      {
        if(j == 0)
          s[i] = 0;
        s[i] += getSample(i);
      }
    for(int i = 0; i < 5; i++)
      s[i] /= sampleAvg;
      
    for(int i = 0; i < 5; i++)
    {
      Serial.print(s[i]);
      Serial.print(' ');
    }
    const int pattern[6][5] = {
      {100, 120, 60, 100, 120}, //invalid reading below
      {100, 120, 90, 100, 120}, //20%   
      {100, 160, 120, 125, 120}, //40%
      {100, 100, 160, 150, 140}, //60%
      {100, 100, 100, 180, 170}, //80%
      {100, 100, 100, 185, 190}};//100%
    
    int maxp = 6;
    for(int p = 0; p < 6; p++) //pattern boundary
      for(int i = 0; i < 5; i++)  //channels
        if(pattern[p][i] > s[i]) maxp = min(p, maxp);
    Serial.print((maxp - 1) * 20);
    Serial.println();
    //set bit
    minMaxBits |= 1 << maxp;
  }
  return minMaxBits;
}

//0: charging, 1:discharging, 3:stop
int mode = 0;

void loop()
{
  if(digitalRead(0) == 0) mode = 1;
  Serial.print("Mode ");
  Serial.println(mode);
  Serial.println("Checking display");
  pushButton(0);
  delay(1000);
  Serial.println("reading");
  int minMax = scanMinMax();
  Serial.print("Bits ");
  Serial.println(minMax, 2);
  int charge = 0;

  /*if(minMax & 0b000001)
  {
    //error no visual
    mode = 3;
  }*/

  switch(mode)
  {
    case 0: //charging
      if(minMax == 0b1000000 || minMax == 0b1100000 || minMax == 0b0100000)
      {
        //above 80%, discharge
        Serial.println("Let's discharge");
        pushButton(1);  //turn on
        mode = 1;
      }
      delay(20000);
      break;
    case 1: //discharging
      if(minMax == 0b0000100 || minMax == 0b0000100 || minMax == 0b0000110)
      {
        Serial.println("Let's charge");
        //batt 20% or below
        pushButton(1);  //turn off
        mode = 0;
        delay(60000);
      }
      break;
    case 2: //charging
      //call home
      while(true) delay(1000);
  }
}
