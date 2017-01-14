S01R01.txt is an original dataset file.

Running the Python script Modify_Dataset_File.py takes a text file as input and outputs three new versions of the file:
1) S01R01_exp.txt: vertical acceleration data for the entire experiment (both normal gait and frozen gait)
2) S01R01_normal.txt: vertical acceleration data for normal gait only
3) S01R01_freeze.txt: vertical acceleration data for freeze of gait only

The Processing script takes one of these text files as input (modify the code to select which one to use).

The Arduino code PrototypeTestDataset.ino is a modified version that takes input using Serial.read instead of Analog.read. Connect the Arduino to the computer and make sure it is running before starting the Processing code to send over data. Serial.print output will be displayed in Processing.
