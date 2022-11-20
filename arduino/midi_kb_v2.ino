/*

  0 - 11:   Key change
  12 - 50:  Notes
  51 - 52:  Oct up/down
  53:       Special / Remap

    ================ KEY CODES ================
       15 18 21 24 27 30    34 37 40 43 46 49
    12 14 17 20 23 26 29 31 33 36 39 42 45 48 50 
       13 16 19 22 25 28    32 36 38 41 44 47

    =========== KEY CODES (from 0) ============
       3  6  9  12 15 18    22 25 28 31 34 37
    0  2  5  8  11 14 17 19 21 24 27 30 33 36 38 
       1  4  7  10 13 16    20 23 26 29 32 35

    ================ NOTE NUMS ================
       3  5  6  8  10 12    15 17 18 20 22 24
    0  2  4  5  7  9  11 12 14 16 17 19 21 23 24  
       1  3  4  6  8  10    13 15 16 18 20 22



  120 = Oct +5
  108 = Oct +4
  96 = Oct +3
  84 = Oct +2
  72 = Oct +1
  60 = Middle C = Oct 0
  48 = Oct -1
  36 = Oct -2
  24 = Oct -3
  12 = Oct -4
  0  = Oct -5


  C  = 0
  C# = 1
  D  = 2
  D# = 3
  E  = 4
  F  = 5
  F# = 6
  G  = 7
  G# = 8
  A  = 9
  A# = 10
  B  = 11

  SendNote = MIDIROOT + Key + (Octave * 12) + NoteSend 

*/
#include <Keypad.h>
#include <USB-MIDI.h>

USBMIDI_CREATE_DEFAULT_INSTANCE();

const byte ROWS = 9;  //four rows
const byte COLS = 6;  //three columns
const int MIDIROOT = 60;

int octave = 0;
int rootkey = 0; // 0 = C, 2 = C#, ..., 11 = B

char keys[ROWS][COLS] = {
  { '0', '1', '2', '3', '4', '5' },
  { '6', '7', '8', '9', 'A', 'B' },
  { 'C', 'D', 'E', 'F', 'G', 'H' },
  { 'I', 'J', 'K', 'L', 'M', 'N' },
  { 'O', 'P', 'Q', 'R', 'S', 'T' },
  { 'U', 'V', 'W', 'X', 'Y', 'Z' },
  { '!', '@', '#', '$', '%', '^' },
  { '&', '*', '(', ')', '-', '_' },
  { '=', '+', '`', '~', '/', '?' },
};

/*
    ================ NOTE NUMS ================

    ============== DEFAULT CONFIG =============
       3  5  6  8  10 12    15 17 18 20 22 24
    0  2  4  5  7  9  11 12 14 16 17 19 21 23 24  
       1  3  4  6  8  10    13 15 16 18 20 22

    ============= MINOR KEY EXAMPLE ===========
       3  4  6  8  9  11    15 16 18 20 21 23
    0  2  3  5  7  8  10 12 14 15 17 19 20 22 24  
       1  2  4  6  7  9     13 14 16 18 19 21

    ================ KEY CODES ================
       15 18 21 24 27 30    34 37 40 43 46 49
    12 14 17 20 23 26 29 31 33 36 39 42 45 48 50 
       13 16 19 22 25 28    32 35 38 41 44 47

*/
// Offset kcode by -12 when accessing this array
int default_kcodeToNoteCode[39] = {0, 1, 2, 3, 3, 4, 5, 4, 5, 6, 6, 7, 8, 8, 9, 10, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 16, 17, 18, 18, 19, 20, 20, 21, 22, 22, 23, 24, 24};
int kcodeToNoteCode[39];

// Offset kcode by -12 when accessing this array
int kcodeToRelativeScaleDegree[39] = {0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7};
int scaleDegreeDefaultNoteValue[15] = {0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23, 24};
int scaleDegreeToKcodes[8][6] {
  {12, 31, 50, 12, 31, 50},
  {13, 14, 15, 32, 33, 34},
  {16, 17, 18, 35, 36, 37},
  {19, 20, 21, 38, 39, 40},
  {22, 23, 24, 41, 42, 43},
  {25, 26, 27, 44, 45, 46},
  {28, 29, 30, 47, 48, 49},
  {12, 31, 50, 12, 31, 50}
};

int notePlaying[39] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

bool specialKeyHeld = false;

int last3Keys[3] = {-1, -1, -1};

byte rowPins[ROWS] = { 1, 0, 2, 3, 4, 5, 6, 7, 8 };  //connect to the row pinouts of the keypad
byte colPins[COLS] = { 18, 15, 14, 16, 10, 9 };      //connect to the column pinouts of the keypad

Keypad kpd = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void apply_default_kcodeToNoteCode(){
  for(int i = 0; i < 39; i++){
    kcodeToNoteCode[i] = default_kcodeToNoteCode[i];
  }
}

void setup() {
  Serial.begin(9600);
  MIDI.begin(1);

  apply_default_kcodeToNoteCode();
}

void loop() {
  if (kpd.getKeys()) {
    for (int i = 0; i < LIST_MAX; i++)  // Scan the whole key list.
    {
      if (kpd.key[i].stateChanged)  // Only find keys that have changed state.
      {
        switch (kpd.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
          case PRESSED:
            buttonPressed(kpd.key[i]);
            break;
          case HOLD:
            break;
          case RELEASED:
            buttonReleased(kpd.key[i]);
            break;
          case IDLE:
            break;
        }
      }
    }
  }
}  // End loop

bool canOctaveUp() {
  int highestnote = MIDIROOT + rootkey + ((octave + 1) * 12) + 24;

  return highestnote < 127;  
}

bool canOctaveDown() {
  int lowestnote = MIDIROOT + rootkey + ((octave - 1) * 12);

  return lowestnote > 0;
}

void octaveUp() {
  if(canOctaveUp()){
    octave++;
  }
}

void octaveDown() {
  if(canOctaveDown()){
    octave--;
  }
}

void modifyKeyMap(Key key) {
  // Check which scale degree kcode belongs to, then take action whether it is sharp or flat or center. center = return to default. 
  // kcodeToRelativeScaleDegree
  // scaleDegreeDefaultNoteValue
  // scaleDegreeToKcodes
  // noteQuality -- -1 = flat, 0 = default, 1 = sharp
  int kcode = key.kcode;
  if(kcode - 12 > 19){
    kcode = kcode - 19;
  }
  int scaleDegree = kcodeToRelativeScaleDegree[kcode - 12];
  int noteQuality = default_kcodeToNoteCode[kcode - 12] - scaleDegreeDefaultNoteValue[scaleDegree];

  // As long as the scale degree selected is NOT the first scale degree, continue
  if(scaleDegree != 0 && scaleDegree != 7 && scaleDegree != 14) { 
    for(int i = 0; i < 6; i++){
      int current_kcode = scaleDegreeToKcodes[scaleDegree][i];
      if (current_kcode != -1) {
        kcodeToNoteCode[current_kcode - 12] = default_kcodeToNoteCode[current_kcode - 12] + noteQuality;
      }
    }
  }

  Serial.print("New Scale Layout: ");
  for(int i = 0; i < (sizeof kcodeToNoteCode) / (sizeof kcodeToNoteCode[0]); i ++){
    Serial.print(kcodeToNoteCode[i]);
    Serial.print(", ");
  }
  Serial.println();
  
}

void logKey(Key key) {
  last3Keys[2] = last3Keys[1];
  last3Keys[1] = last3Keys[0];
  last3Keys[0] = key.kcode;
}

void buttonPressed(Key key) {
  logKey(key);

  if(key.kcode < 12) {
    keyPressed(key);
  } else if(key.kcode < 51) {
    if(specialKeyHeld){
      modifyKeyMap(key);
    } else {
      notePressed(key);
    }
  } else if(key.kcode == 51) {
    octaveUp();
  } else if(key.kcode == 52) {
    octaveDown();
  } else if(key.kcode == 53) {
    specialPressed();
  }
}

void buttonReleased(Key key) {
  // We care only if the released key is a note key
  // OR SPECIAL KEY
  if(key.kcode >= 12 && key.kcode <= 50
  && notePlaying[key.kcode - 12] > -1
  && !specialKeyHeld) {
    noteReleased(notePlaying[key.kcode - 12], key);
  } else if(key.kcode == 53) {
    specialReleased();
  }
}

void notePressed(Key key) {
  int note = MIDIROOT + rootkey + (octave * 12) + kcodeToNoteCode[key.kcode - 12];
  notePlaying[key.kcode - 12] = note;
  MIDI.sendNoteOn(note, 127, 1);
}

void noteReleased(int note, Key key) {
  notePlaying[key.kcode - 12] = -1;
  MIDI.sendNoteOff(note, 127, 1);
}

void keyPressed(Key key) {
  // Check if target key is valid based on highest + lowest notes
  // Too high = lower octave
  // Too low = raise octave

  int highestnote = MIDIROOT + key.kcode + (octave * 12) + 24;
  int lowestnote = MIDIROOT + key.kcode + (octave * 12);

  while(highestnote > 127) {
    octave--;
    highestnote = MIDIROOT + key.kcode + (octave * 12) + 24;
  }
  while(lowestnote < 0) {
    octave++;
    lowestnote = MIDIROOT + key.kcode + (octave * 12);
  }
  rootkey = key.kcode;
}

void keyReleased(Key key) {
  // Not currently used, may change w/ new features later
}

void specialPressed() {
  specialKeyHeld = true;

  if(  last3Keys[0] == 53
    && last3Keys[1] == last3Keys[0]
    && last3Keys[2] == last3Keys[1]){
      apply_default_kcodeToNoteCode();
    }
}

void specialReleased(){
  specialKeyHeld = false;
}
