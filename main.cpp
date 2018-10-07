/* EE201A Winter 2018 Course Project
 */

#include <iostream>
#include <fstream>      //to write/read files
#include <algorithm>    // max and min functions 
#include <math.h>       //import sqrt() and pow()
#include "oaDesignDB.h"
#include <vector>
#include "InputOutputHandler.h"
#include "ProjectInputRules.h"
#include "OAHelper.h"

using namespace oa;
using namespace std;

static oaNativeNS ns;



// ****************************************************************************
//
//  INPUTS: - arrayString is an array of oaStrings
//          - instName is the name of the instance we want to check whether it is in arrayString or not
//          - size is the max index value until when we will look for instName in arrayString[]
//
//  OUTPUT: if instName in arrayString, returns its index in the array, and -1 otherwise.
// ****************************************************************************
int findString(oaString arrayString[], oaString instName, int size) {
        
        for(int i=0; i < size; i++) {
              if(instName==arrayString[i]) {
                      return i;   //we found string in array
              }
        }
        
   return -1;
}



// ****************************************************************************
//
//  INPUTS: - masterName: name of master of macro 
//          - instName: name of macro instance
//
//  OUTPUT: the name of the macro master
oaString masterName(oaString masterName, oaString instName) {  
    int indexInstName = masterName.substr(instName);
    
    oaString newName;
    
    for(int i=0; i<indexInstName; i++) {
       newName += masterName[i];
    }

    return newName;
}

//Return the angle between orientation of orient1 and orient2 (take as reference: angle 0°) 
//orients are names starting with "R" and then value of the angle
int diffAngle (oaString orient1, oaString orient2) {
    
    oaString angle1;
    for(int i=1; i<orient1.getLength(); i++) {
       angle1 += orient1[i];
    }
    oaString angle2;
    for(int i=1; i<orient2.getLength(); i++) {
       angle2 += orient2[i];
    }
    
    int angle_1;
    int angle_2;
    
    if(angle1=="90") { angle_1=90; }
    else if(angle1=="180") { angle_1=180;}
    else if(angle1=="270") { angle_1=270;}
    else { angle_1=0; }
    
    if(angle2=="90") { angle_2=90; }
    else if(angle2=="180") { angle_2=180;}
    else if(angle2=="270") { angle_2=270;}
    else { angle_2=0; }

    if(angle_1-angle_2>=0) {
          return angle_1-angle_2;
    }
    else {
          return 360+angle_1-angle_2;
    }

    
}

/*
 * 
 */
int main(int argc, char *argv[])
{
    //Hello World
    cout << "=================================================" << endl;
    cout << "Automated Inter-Chip Pin Assignment" << endl;
    cout << "UCLA EE 201A Winter 2017 Course Project" << endl;
    cout << endl;
    cout << "6" << endl;
    cout << "Tiefang Li & Zaurbek Tsorojev" << endl;
    cout << "<205035362 & 805029443>" << endl;
    cout << "=================================================" << endl << endl;
   
    //Usage
    cout << "Ensure you have an existing OA design database before running this tool. Also please adhere to the following command-line usage:" << endl;
    cout << "./PinAssign <DESIGN NAME> <OUTPUT DESIGN NAME> <INPUT RULE FILE NAME> <MACRO INFO FILENAME>" << endl;
    cout << "For example:" << endl;            
    cout << "./PinAssign sbox_x2 sbox_x2_minrule benchmarks/sbox_x2/min.inputrules logs/sbox_x2/pinassign_sbox_x2_minrule.macros" << endl;

	// Initialize OA with data model 3
	oaDesignInit(oacAPIMajorRevNumber, oacAPIMinorRevNumber, 3);
    oaRegionQuery::init("oaRQSystem");

    //Read in design library
    cout << "\nReading design library..." << endl;
    DesignInfo designInfo;
    InputOutputHandler::ReadInputArguments(argv, designInfo);
	  oaLib* lib;
    oaDesign* design= InputOutputHandler::ReadOADesign(designInfo, lib);

	// Get the TopBlock for this design.
    oaBlock* block= InputOutputHandler::ReadTopBlock(design);

	// Fetch all instances in top block and save a unique master design copy for each
    cout << "\nSaving copies of each unique macro instance..." << endl;
	InputOutputHandler::SaveMacroDesignCopies(designInfo, block);
	
    //now, get the input rules from file
    cout << "\nReading input rules..." << endl;
    ProjectInputRules inputRules(designInfo.inputRuleFileName); 
    inputRules.print();
    
    cout << "\nBeginning pin assignment..." << endl;
	//=====================================================================
  // All pin assignment code should be handled here
	// The scratch code below covers basic traversal and some useful functions provided
	// You are free to edit everything in this section (marked by ==)

//*********************************************************
//ATTENTION: UNITS ARE IN MICRONS - we convert in DBU: 1um = 2000DBU
  float maxPertubation = inputRules.getMaxPinPerturbation()*2000;  
  float minMoveStep = inputRules.getPinMoveStep()*2000;
  float minPinPitch = inputRules.getMinPinPitch()*2000; 
//*********************************************************

        
  oaBox topBox;
  block->getBBox(topBox);
  
  //if the perturbation is negative, it means that it's infinite so we don't take it into account (we give it an upper bound)
  if (maxPertubation<0) {
          maxPertubation = topBox.getWidth() + topBox.getHeight();
  }
  
  
  int x = 0;
  int y = 0;

  int outerPinsCount = 0;    //counter of outer pins

  int countPins = 0;
  
  //when pin connected to multiple outer pins, take average of all outer pin positions to find direction to move your pin
  int x_outerPin = 0;
  int y_outerPin = 0;
  double x_avg = 0.0;
  double y_avg = 0.0;
  
  int pinWidth;
  int pinHeight;
  
  int index = 0;
  
  //coordinates of macro edges
  int top;
  int bottom;
  int right;
  int left;
  
  int initialY;
  int initialX;
  
  
  int nInst = (block->getInsts()).getCount();    //count number of macros

  oaString netName, instName, instMName, currentInstTermName, outerInstTermName, masterCellName, assocTermName, termName, cellNameCurrent, masterInstName;
  oaString masterCellNameCurrent, instNameCurrent, masterName1, instTermSameMacroName;
  
  int nameCount = 0;
  
  int countMacros = 0;
  
  int nMasterMacros = (block->getInstHeaders()).getCount();	
  //outerCircle << "nMasterMacros: " << nMasterMacros << endl;
  
  oaPoint  ptCenter;
  oaBox bBox;
  
  
  //define center and bouding box of macro with same master than current macro
  oaPoint  centerSameMacro;
  oaBox bBoxSameMacro; 
  
  
  
  //stores all the macro's masters in the design; we size it to reach top boundary (each macro has one individual master)
  oaString macroMasters[nInst];
  int m=0; //counter of macro masters in design


  // Iterate over all macros in the design
  oaIter<oaInst>   cellIterator(block->getInsts());
  while (oaInst * inst = cellIterator.getNext()) {
  
      inst->getCellName(ns, masterCellName);
      inst->getName(ns, instName);
      
      masterInstName = masterName(masterCellName, instName);
      
      //GET CENTER OF MACRO
      inst->getBBox(bBox);      //returns in bBox, the bounding box of the macro
      bBox.getCenter(ptCenter);   //we get the center of bBox as the approximate position of macro's center
      
      

      
      
      //if pin assignement already done for master of this macro, simply rotate it
      if(findString(macroMasters, masterInstName, m)>=0) {
            
            //****************
            //We did originally did a rotation of the macro to find the angle with best HPWL, but because it failed 
            //for benchmark aes_top (although it worked perfectly for ALL the other benchmarks), we decided to remove it.
            //Right now, we just keep the macro at the same orientation.
            //****************
      }
      
      //if not, assign the macro for the first time
      else {

            //coordinates of macro edges
            top = bBox.top();
            bottom = bBox.bottom();
            right = bBox.right();
            left = bBox.left();
            
            //count the number of pins of the current macro
            countPins = (inst->getInstTerms()).getCount();
            
            
            oaInstTerm* pinsInstTerm[countPins];              //stores instTerms pointers
            oaPoint pinsPositions[countPins];                 //stores the orginial order of pins with their associated positions
            oaString pinsNames[countPins];                    //stores names of pins
            
            
            int pinDirections[countPins];                     //this array matches the pinsPositions array
                                                              //Gives direction where we have to move each pin
                                                               
            int pinEdge[countPins];                           //Gives edge of pin on macro; this array matches the other two
            
            
            int invalidPositions[2][countPins];               //Contains for each pin the boundaries for closest where you can put another pin 
                                                              //(inside theses boundaries is invalid       positions for any ohter pin)
                                                              //1st line: contains coordinate of top (y) or right (x) boundary
                                                              //2nd line: contains coordinate of bottom (y) or left (x) boundary
                                                              

     
            int i = 0;    //index of pinsPositions
            

            //for every pin of the current macro
            oaIter<oaInstTerm> instTermIterator(inst->getInstTerms());
            while (oaInstTerm* instTerm = instTermIterator.getNext()) {

            		oaPoint instTermPos = OAHelper::GetAbsoluteInstTermPosition(instTerm);
                instTerm->getTermName(ns, currentInstTermName);
                  
                  
                  //every pin of given macro have some size so we only do this once
                   if(i==0) { 
                          //Get width and height of pinFig BB
                          oaIter<oaPin> pinIterator(instTerm->getTerm()->getPins());
                        	oaIter<oaPinFig> pinFigIterator(pinIterator.getNext()->getFigs());
                        	oaPinFig* pinFig = pinFigIterator.getNext();
                        	oaBox boxPin;
                        	pinFig->getBBox(boxPin);
                          pinWidth = boxPin.getWidth();
                          pinHeight = boxPin.getHeight();
                   }
                  
                
                  
                //if pin is an internal pin
                if(currentInstTermName=="VSS" || currentInstTermName=="VDD")  {
                      //we don't do anything 
                }
                
                //if boundary pin
                else { 
                
                    pinsNames[i] = currentInstTermName;
                  
  
                    
                
                    pinsInstTerm[i] = instTerm;
                    pinsPositions[i] = instTermPos;
                  
                  
                    invalidPositions[0][i] = 0;
                    invalidPositions[1][i] = 0;
                  
            //******************************************************************************
            //WE WANT TO KNOW ON WHICH EDGE OF MACRO IS EACH PIN 
            //so that we can know in which exact direction to move it
            //******************************************************************************

              //CHECK IF PUTTING EQUALITY OK OR SHOULD WE PUT A MARGIN
                    if(instTermPos.y()+pinHeight/2 == top) {
                            pinEdge[i] = 1;     //top

                    }
                    else if(instTermPos.y()-pinHeight/2 == bottom) {
                            pinEdge[i] = 2;     //bottom
 
                    }
                    else if(instTermPos.x()+pinWidth/2 == right) {
                            pinEdge[i] = 3;      //right                            
                            
                    }
                    else if(instTermPos.x()-pinWidth/2 == left) {
                            pinEdge[i] = 4;      //left
                    }
                  
                  
                  
  //*****************************************
  //FIND DIRECTIONS (outer macros)
  //*****************************************                
                  
                  

                  //a new array will be created at each loop iteration (so that it is reinitialized)
                  oaString tempName[nInst];
                  
                  //contains all macro centers
                  oaPoint macroCenters[nInst];
                  
                  //for the net connected to current pin
	                oaNet* net = instTerm->getNet();

                  nameCount = 0;
                  
                  oaPoint  MacroCenter;
                  
                  //for every pin connected to this net, find all macros this net is connected to
                  //we take both the pins of current macro and outer macro
                  oaIter<oaInstTerm> instTermIterator2(net->getInstTerms());
                  while (oaInstTerm* instTermOuter = instTermIterator2.getNext()) {

                        instTermOuter->getInst()->getName(ns, instMName);
                        
                          
                        outerPinsCount++;
                        
                        oaPoint instTermOuterPos = OAHelper::GetAbsoluteInstTermPosition(instTermOuter);
                      
                        //*************************************************   
                        //when pin connected to multiple outer pins, take average of all outer pin positions to find direction to move your pin
                        //accumulated x and y positions (for average later)
                        x_outerPin += instTermOuterPos.x();
                        y_outerPin += instTermOuterPos.y();

   
                  }//end of while: find all outer pins
                  
                  
                  //find all terminals connected to this net
                  oaIter<oaTerm> termIterator(net->getTerms());
                  while (oaTerm* term = termIterator.getNext()) {
                  
                        oaPoint termPos = OAHelper::GetTermPosition(term);
                        
                        x_outerPin += termPos.x();
                        y_outerPin += termPos.y();
                        
                        outerPinsCount++;
                  }
                  
                  
                  outerPinsCount--; //we don't want to count the current boundary pin
                  
                  //we remove positions from current pin out of accumulated positions
                  x_outerPin -= instTermPos.x();
                  y_outerPin -= instTermPos.y();
                  
                  x_avg = x_outerPin/(double)outerPinsCount;
                  y_avg = y_outerPin/(double)outerPinsCount;
                  
                  
                  //we re-initialize
                  outerPinsCount = 0;   
                  x_outerPin = 0;
                  y_outerPin = 0;
                  
          //*************************
          //DEFINE DIRECTIONS WHERE WE WILL HAVE TO MOVE PINS according to code:
          //            top:      1
          //            bottom:   2
          //            right:    3
          //            left:     4
          //************
          //You can also combine them. (e.g. top-right: 13)
          //*************************
                  
                  //Initialization
                  pinDirections[i] = 0;
                  
                  //top
                  if((int)y_avg > instTermPos.y() && (pinEdge[i] == 3 || pinEdge[i] == 4)) {    //if outer macro is at the top and pin not on top edge of current macro, we move pin to top
                          pinDirections[i] = 1;
                  }
                  
                  //bottom
                  else if((int)y_avg < instTermPos.y() && (pinEdge[i] == 3 || pinEdge[i] == 4)) {    //bottom
                          pinDirections[i] = 2;
                  }
                  
                  //right, top-right or bottom-right
                  if((int)x_avg > instTermPos.x() && (pinEdge[i] == 1 || pinEdge[i] == 2)) {         //right (if it's only right, it will be 3 because pinDirections[i] will be zero previously
                                                                                  //int arrays are initially at 0 by default
                                          
                          //pinDirections[i] = pinDirections[i]*10 + 3;
                          pinDirections[i] = 3;       
                  }
                  
                  //left, top-left or bottom-left
                  else if((int)x_avg < instTermPos.x() && (pinEdge[i] == 1 || pinEdge[i] == 2)) {    //left
                          //pinDirections[i] = pinDirections[i]*10 + 4;
                          pinDirections[i] = 4;
                  }
                  
                  
                  //***********************
                  //If you want to use every possible directions and not only four, uncomment this section
                  /*
                  //top
                  if((int)y_avg > instTermPos.y() && pinEdge[i] != 1) {    //if outer macro is at the top and pin not on top edge of current macro, we move pin to top
                          pinDirections[i] = 1;
                  }
                  
                  //bottom
                  else if((int)y_avg < instTermPos.y() && pinEdge[i] != 2) {    //bottom
                          pinDirections[i] = 2;
                  }
                  
                  //right, top-right or bottom-right
                  if((int)x_avg > instTermPos.x() && pinEdge[i] != 3) {         //right (if it's only right, it will be 3 because pinDirections[i] will be zero previously
                                                                                  //int arrays are initially at 0 by default
                                          
                          pinDirections[i] = pinDirections[i]*10 + 3;
                                 
                  }
                  
                  //left, top-left or bottom-left
                  else if((int)x_avg < instTermPos.x() && pinEdge[i] != 4) {    //left
                          pinDirections[i] = pinDirections[i]*10 + 4;
                  }
                  
                  */
                  
                  
                  
                  i++;    //increment pin index; it counts the number of boundary pins of current macro

             } //end of if: not internal pin 
              
         } //end of while: for every pin of the current macro
         
         
             //countPins will only contain the number of BOUNDARY pins
             countPins = i; 
            
          
             //find valid new position and move the pin
             int maxmove= int (floor(maxPertubation/minMoveStep)); //max we can move the pin (taking into account the perturbation)
             int v,v1,v2;        //if invalid positions, we increment by one
             int valid;          //if =1, means valid position for pin placement
             int maxs;           //maximum legal move as a multiple of the step

             
             
             //for every pin
             for (int i=0;i<countPins;i=i+1){
             
                 initialY=pinsPositions[i].y();
                 initialX=pinsPositions[i].x();
                 
                 //move up--------------
                 if (pinDirections[i]==1){
                     if (float(pinsPositions[i].y())>float(top-pinHeight/2-minPinPitch/2)){
                         maxs=int(floor(float(pinsPositions[i].y()-(top-pinHeight/2-minPinPitch/2))/minMoveStep))+1;
                         
                         pinsPositions[i].y()-=int(maxs*minMoveStep);
                     }
                     //we will move the pin to max perturbation position
                     else if  (float(pinsPositions[i].y()+maxmove*minMoveStep)>float(top-pinHeight/2-minPinPitch/2)){
                         maxs=int(floor(float(top-pinHeight/2-minPinPitch/2-pinsPositions[i].y())/minMoveStep));
                         
                         pinsPositions[i].y()+=int(maxs*minMoveStep);
                     }

                     else {
                         maxs=maxmove;
                         
                         pinsPositions[i].y()=int(maxs*minMoveStep)+pinsPositions[i].y();
                     }
                     v=0;
                     valid = 0;
                     

                     
                     //while pin on invalid position, we will move it out of invalid region
                     while (valid==0){
                         //for every step we can go down
                         for (int n=0;n<maxs;n=n+1){
                             //for every pin
                             for (int c=0;c<countPins;c=c+1){
                             
                               
                                 //if we pin is in invalid position (we check for right edge (3) and left edge (4) of macro)
                                 if ((pinsPositions[c].x()==left+pinHeight/2)&&(pinsPositions[i].x()==left+pinHeight/2)&&(pinsPositions[i].y()<=invalidPositions[0][c])&&(pinsPositions[i].y()>=invalidPositions[1][c])){
                                     v++;                                                                           
                                 }
                                 else if ((pinsPositions[c].x()==right-pinHeight/2)&&(pinsPositions[i].x()==right-pinHeight/2)&&(pinsPositions[i].y()<=invalidPositions[0][c])&&(pinsPositions[i].y()>=invalidPositions[1][c])){
                                     v++; 
                                     //innerCircle <<  "invalidPositions - Top: " <<   invalidPositions[1][c] << ", " <<  invalidPositions[0][c]  << endl;                                                              
                                 }
                                 else if((pinsPositions[c].y()==bottom+pinHeight/2)&&(pinsPositions[i].y()==bottom+pinHeight/2)&&(pinsPositions[i].x()<=invalidPositions[0][c])&&(pinsPositions[i].x()>=invalidPositions[1][c])) {
                                     v++;
                                 }
                             }
                             
                             //if v hasn't been incremented at this stage, we are in a valid position
                             if (v==0){
                                 
                                 if(pinsPositions[i].y()-pinHeight/2 == bottom) {
                                          invalidPositions[0][i] = pinsPositions[i].x()+pinWidth + minPinPitch;
                                          invalidPositions[1][i] = pinsPositions[i].x()-pinWidth - minPinPitch;
                                   }
                                   else {
                                         invalidPositions[0][i] = pinsPositions[i].y()+pinHeight + minPinPitch;
                                         invalidPositions[1][i] = pinsPositions[i].y()-pinHeight - minPinPitch;
                                   } 
                                 //innerCircle <<  "invalidPositions - Top: " <<   invalidPositions[1][i] << ", " <<  invalidPositions[0][i]  << endl;
                                 valid=1;
                                 break;   
                             }
                             
                             //else: we have to move pin down
                             else {
                                 if(pinsPositions[i].y() > bottom+pinHeight/2+minPinPitch/2+minMoveStep) {
                                     pinsPositions[i].y() -= minMoveStep; 
                                 }
                                 else if ((pinEdge[i]==4)&&(pinsPositions[i].x()>=left+pinHeight/2 +((int((initialY-bottom+pinHeight + minPinPitch)/minMoveStep)+1)*minMoveStep-(initialY-bottom-pinHeight/2)))){
                                     pinsPositions[i].x() += minMoveStep; 
                                 }
                                 else if ((pinEdge[i]==3)&&(pinsPositions[i].x()<=right-pinHeight/2-((int((initialY-bottom+pinHeight + minPinPitch)/minMoveStep)+1)*minMoveStep-(initialY-bottom-pinHeight/2)))){
                                     pinsPositions[i].x() -= minMoveStep;
                                 }
                                 else {
                                     
                                     pinsPositions[i].y() = bottom+pinHeight/2;
                                     if(pinEdge[i]==4) {
                                       pinsPositions[i].x()=left+pinHeight/2 +((int((initialY-bottom+pinHeight + minPinPitch)/minMoveStep)+1)*minMoveStep-(initialY-bottom-pinHeight/2));
                                      
                                     }
                                     else if(pinEdge[i]==3) {
                                       pinsPositions[i].x()=right-pinHeight/2-((int((initialY-bottom+pinHeight + minPinPitch)/minMoveStep)+1)*minMoveStep-(initialY-bottom-pinHeight/2));
                                       //innerCircle <<  "-------------" <<   pinsPositions[i].x()  << endl;
                                     }
                                 }
                                 v=0;
                             }
                         }//end of for loop (steps)
                         
                     }//end of while loop     
                                     
                 }//end of if (we have to move pin to top)
                 
                 
                 
                 //move down--------------
                 if (pinDirections[i]==2){
                 
                     if (float(pinsPositions[i].y())<float(bottom+pinHeight/2+minPinPitch/2)){
                         maxs=int(floor(float(bottom+pinHeight/2+minPinPitch/2-pinsPositions[i].y()))/minMoveStep)+1;
                         //innerCircle << "maxs: " << maxs << endl;
                         pinsPositions[i].y()+=int(maxs*minMoveStep);
                     }
                     
                     else if  (float(pinsPositions[i].y()-maxmove*minMoveStep)<float(bottom+pinHeight/2+minPinPitch/2)){
                         maxs=int(float(pinsPositions[i].y() - bottom - pinHeight/2 - minPinPitch/2)/minMoveStep);
                         pinsPositions[i].y()=pinsPositions[i].y()-int(maxs*minMoveStep);
                         
                     }
                     else {
                         maxs=maxmove;
                         pinsPositions[i].y()=pinsPositions[i].y()-int(maxs*minMoveStep);
                         
                     }
                     
                     v=0;
                     valid = 0;

                     
                     
                     while (valid==0){
                         for (int n=0;n<maxs;n=n+1){
                             for (int c=0;c<countPins;c=c+1){
                                 if ((pinsPositions[c].x()==left+pinHeight/2)&&(pinsPositions[i].x()==left+pinHeight/2)&&(pinsPositions[i].y()<=invalidPositions[0][c])&&(pinsPositions[i].y()>=invalidPositions[1][c])){
                                     v=v+1;                                                                           
                                 }
                                 else if ((pinsPositions[c].x()==right-pinHeight/2)&&(pinsPositions[i].x()==right-pinHeight/2)&&(pinsPositions[i].y()<=invalidPositions[0][c])&&(pinsPositions[i].y()>=invalidPositions[1][c])){
                                     v=v+1;  
                                                                                                             
                                 }
                                 else if((pinsPositions[c].y()==top-pinHeight/2)&&(pinsPositions[i].y()==top-pinHeight/2)&&(pinsPositions[i].x()<=invalidPositions[0][c])&&(pinsPositions[i].x()>=invalidPositions[1][c])) {
                                     v++;
                                 }
                             }
                             //innerCircle <<  "v2--------" <<   v  << endl;
                             if (v==0){
                                 if(pinsPositions[i].y()+pinHeight/2 == top) {
                                          invalidPositions[0][i] = pinsPositions[i].x()+pinWidth + minPinPitch;
                                          invalidPositions[1][i] = pinsPositions[i].x()-pinWidth - minPinPitch;
                                   }
                                   else {
                                         invalidPositions[0][i] = pinsPositions[i].y()+pinHeight + minPinPitch;
                                         invalidPositions[1][i] = pinsPositions[i].y()-pinHeight - minPinPitch;
                                   } 
                                 valid=1;
                                 
                                 break;
                             }
                             else {
                                 if(pinsPositions[i].y() < top-pinHeight/2-minPinPitch/2-minMoveStep) {
                                     pinsPositions[i].y() += minMoveStep; 
                                 }
                                 else if ((pinEdge[i]==4)&&(pinsPositions[i].x()>=left+pinHeight/2 +((int((top-initialY+pinHeight + minPinPitch)/minMoveStep)+1)*minMoveStep-(top-pinHeight/2-initialY)))){
                                     pinsPositions[i].x() += minMoveStep; 
                                 }
                                 else if ((pinEdge[i]==3)&&(pinsPositions[i].x()<=right-pinHeight/2-((int((top-initialY+pinHeight + minPinPitch)/minMoveStep)+1)*minMoveStep-(top-pinHeight/2-initialY)))){
                                     pinsPositions[i].x() -= minMoveStep;
                                 }
                                 else {
                                     
                                     pinsPositions[i].y() = top-pinHeight/2;
                                     if(pinEdge[i]==4) {
                                       pinsPositions[i].x()=left+pinHeight/2 +((int((top-initialY+pinHeight + minPinPitch)/minMoveStep)+1)*minMoveStep-(top-pinHeight/2-initialY));
                                      
                                     }
                                     else if(pinEdge[i]==3) {
                                       pinsPositions[i].x()=right-pinHeight/2-((int((top-initialY+pinHeight + minPinPitch)/minMoveStep)+1)*minMoveStep-(top-pinHeight/2-initialY));
                                       //innerCircle <<  "-------------" <<   pinsPositions[i].x()  << endl;
                                     }
                                 }
                                 
                                 v=0;
                                 
                             }
                         }
                     }  
                 
                 }

                 
                 //move right--------------
                 if (pinDirections[i]==3){
                 
                     if (float(pinsPositions[i].x())>float(right-pinHeight/2-minPinPitch/2)){
                         maxs=int(floor(float(pinsPositions[i].x()-(right-pinHeight/2-minPinPitch/2))/minMoveStep))+1;
                         //innerCircle << "maxs: " << maxs << endl;
                         pinsPositions[i].x()-=int(maxs*minMoveStep);
                     }
                 
                     
                     else if  (float(pinsPositions[i].x()+maxmove*minMoveStep)>float(right-pinHeight/2-minPinPitch/2)){
                         maxs=int(float(right-pinHeight/2-minPinPitch/2 - pinsPositions[i].x())/minMoveStep);
                         pinsPositions[i].x()=int(maxs*minMoveStep)+pinsPositions[i].x();
                     }
                     else {
                         maxs=maxmove;
                         pinsPositions[i].x()=int(maxs*minMoveStep)+pinsPositions[i].x();
                     }
                     
                     v=0;
                     valid = 0;

                     
                     while (valid==0){
                         for (int n=0;n<maxs;n=n+1){
                             for (int c=0;c<countPins;c=c+1){
                                 if ((pinsPositions[c].y()==bottom+pinHeight/2)&&(pinsPositions[i].y()==bottom+pinHeight/2)&&(pinsPositions[i].x()<=invalidPositions[0][c])&&(pinsPositions[i].x()>=invalidPositions[1][c])){
                                     v=v+1;                                                                           
                                 }
                                 else if ((pinsPositions[c].y()==top-pinHeight/2)&&(pinsPositions[i].y()==top-pinHeight/2)&&(pinsPositions[i].x()<=invalidPositions[0][c])&&(pinsPositions[i].x()>=invalidPositions[1][c])){
                                     v=v+1;                                                                           
                                 }
                                 else if((pinsPositions[c].x()==left+pinHeight/2)&&(pinsPositions[i].x()==left+pinHeight/2)&&(pinsPositions[i].y()<=invalidPositions[0][c])&&(pinsPositions[i].y()>=invalidPositions[1][c])) {
                                     v++;
                                 }
                             }
                             
                             if (v==0){
                                 if(pinsPositions[i].x()-pinHeight/2 == left) {
                                          invalidPositions[0][i] = pinsPositions[i].y()+pinWidth + minPinPitch;
                                          invalidPositions[1][i] = pinsPositions[i].y()-pinWidth - minPinPitch;
                                   }
                                   else {
                                         invalidPositions[0][i] = pinsPositions[i].x()+pinHeight + minPinPitch;
                                         invalidPositions[1][i] = pinsPositions[i].x()-pinHeight - minPinPitch;
                                   } 
                                 valid=1;
                                 break;
                             }
                             else {
                                 if(pinsPositions[i].x() > left+pinHeight/2+minPinPitch/2+minMoveStep) {
                                     pinsPositions[i].x() -= minMoveStep; 
                                 }
                                 else if ((pinEdge[i]==2)&&(pinsPositions[i].y()>=bottom+pinHeight/2 +((int((initialX-left+pinHeight + minPinPitch)/minMoveStep)+1)*minMoveStep-(initialX-left-pinHeight/2)))){
                                     pinsPositions[i].y() += minMoveStep;
                                     //innerCircle <<  "333++++++" << endl; 
                                 }
                                 else if ((pinEdge[i]==1)&&(pinsPositions[i].y()<=top- pinHeight/2 - ((int((initialX-left+pinHeight + minPinPitch)/minMoveStep)+1)*minMoveStep-(initialX-left-pinHeight/2)))){
                                     pinsPositions[i].y() -= minMoveStep;
                                     //innerCircle <<  "333------" << endl;
                                 }
                                 else {
                                     //innerCircle <<  "33333333333" << endl;
                                     pinsPositions[i].x() = left+pinHeight/2;
                                     if(pinEdge[i]==2) {
                                       pinsPositions[i].y()=bottom+pinHeight/2 +((int((initialX-left+pinHeight + minPinPitch)/minMoveStep)+1)*minMoveStep-(initialX-left-pinHeight/2));
                                      
                                     }
                                     else if(pinEdge[i]==1) {
                                       pinsPositions[i].y()=top-pinHeight/2-((int((initialX-left+pinHeight + minPinPitch)/minMoveStep)+1)*minMoveStep-(initialX-left-pinHeight/2));
                                       //innerCircle <<  "-------------" <<   pinsPositions[i].x()  << endl;
                                     }
                                 }
                                 
                                 
                                 v=0;
                                 
                             }
                         }
                     }
                 
                 }
                 
               
                 //move left--------------
                 if (pinDirections[i]==4){
                 
                     if (float(pinsPositions[i].x())<float(left+pinHeight/2+minPinPitch/2)){
                         maxs=int(floor(float(left+pinHeight/2+minPinPitch/2-pinsPositions[i].x()))/minMoveStep)+1;
                         //innerCircle << "maxs: " << maxs << endl;
                         pinsPositions[i].x()+=int(maxs*minMoveStep);
                     }
                     else if  (float(pinsPositions[i].x()-maxmove*minMoveStep)<float(left+pinHeight/2+minPinPitch/2)){
                         maxs=int(float(pinsPositions[i].x() - left - pinHeight/2 - minPinPitch/2)/minMoveStep);
                         pinsPositions[i].x()=pinsPositions[i].x()-int(maxs*minMoveStep);
                     }
                     else {
                         maxs=maxmove;
                         pinsPositions[i].x()=pinsPositions[i].x()-int(maxs*minMoveStep);
                     }
                     
                     v=0;
                     valid = 0;
                     
                     
                     while (valid==0){
                         for (int n=0;n<maxs;n=n+1){
                             for (int c=0;c<countPins;c=c+1){
                                 if ((pinsPositions[c].y()==bottom+pinHeight/2)&&(pinsPositions[i].y()==bottom+pinHeight/2)&&(pinsPositions[i].x()<=invalidPositions[0][c])&&(pinsPositions[i].x()>=invalidPositions[1][c])){
                                     v=v+1;                                                                           
                                 }
                                 else if ((pinsPositions[c].y()==top-pinHeight/2)&&(pinsPositions[i].y()==top-pinHeight/2)&&(pinsPositions[i].x()<=invalidPositions[0][c])&&(pinsPositions[i].x()>=invalidPositions[1][c])){
                                     v=v+1;                                                                           
                                 }
                                 else if((pinsPositions[c].x()==right-pinHeight/2)&&(pinsPositions[i].x()==right-pinHeight/2)&&(pinsPositions[i].y()<=invalidPositions[0][c])&&(pinsPositions[i].y()>=invalidPositions[1][c])) {
                                     v++;
                                 }
                             }
                             //innerCircle <<  "v4--------" <<   v  << endl;
                             if (v==0){
                                 if(pinsPositions[i].x()+pinHeight/2 == right) {
                                          invalidPositions[0][i] = pinsPositions[i].y()+pinWidth + minPinPitch;
                                          invalidPositions[1][i] = pinsPositions[i].y()-pinWidth - minPinPitch;
                                   }
                                   else {
                                         invalidPositions[0][i] = pinsPositions[i].x()+pinHeight + minPinPitch;
                                         invalidPositions[1][i] = pinsPositions[i].x()-pinHeight - minPinPitch;
                                   }
                                 valid=1;

                                 break; 
                             }
                             else {
                                 if(pinsPositions[i].x() < right-pinHeight/2-minPinPitch/2-minMoveStep) {
                                     pinsPositions[i].x() += minMoveStep; 
                                 }
                                 else if ((pinEdge[i]==2)&&(pinsPositions[i].y()>=bottom+pinHeight/2 +((int((right-initialX+pinHeight + minPinPitch)/minMoveStep)+1)*minMoveStep-(right-pinHeight/2-initialX)))){
                                     pinsPositions[i].y() += minMoveStep; 
                                     //innerCircle <<  "4++++++++++" << endl;
                                 }
                                 else if ((pinEdge[i]==1)&&(pinsPositions[i].y()<=top-pinHeight/2-((int((right-initialX+pinHeight + minPinPitch)/minMoveStep)+1)*minMoveStep-(right-pinHeight/2-initialX)))){
                                     pinsPositions[i].y() -= minMoveStep;
                                     //innerCircle <<  "4---------" << endl;
                                 }
                                 else {
                                     //innerCircle <<  "44444444444" << endl;
                                     pinsPositions[i].x() = right-pinHeight/2;
                                     if(pinEdge[i]==2) {
                                       pinsPositions[i].y()=bottom+pinHeight/2 +((int((right-initialX+pinHeight + minPinPitch)/minMoveStep)+1)*minMoveStep-(right-pinHeight/2-initialX));
                                      
                                     }
                                     else if(pinEdge[i]==1) {
                                       pinsPositions[i].y()=top-pinHeight/2-((int((right-initialX+pinHeight + minPinPitch)/minMoveStep)+1)*minMoveStep-(right-pinHeight/2-initialX));
                                       //innerCircle <<  "-------------" <<   pinsPositions[i].x()  << endl;
                                     }
                                 }
                                 

                                 v=0;
                             }
                         }
                     } 
                 
                 }
                 
/* 
                
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------    
//-------------------------------------------------------------------------------------------------------------------------------------------                  
                 
                 //move pin to top-right-----------------
                 if (pinDirections[i]==13){
                      //outerCircle <<"----------------"<<endl;
                     //case 1: for pins on left edge, we move them top first and then right
                     if (pinEdge[i] == 4) {
                       if  (float(pinsPositions[i].y()+maxmove*minMoveStep)>float(top-pinHeight/2-minPinPitch/2)){
                       //since i am not sure how to move the pin(along the boundary of maro or piin's center), may modify the something
                       
                           //if the max move goes beyond macro boundary, we limit it to top-right corner
                           if ( float(pinsPositions[i].x()+maxmove*minMoveStep-(top-pinsPositions[i].y() ))>float(right-pinHeight/2-minPinPitch/2) )
                           {
                               maxs=int(floor(float(right-pinHeight/2-minPinPitch/2-left+top-pinsPositions[i].y())/float(minMoveStep)));
                               pinsPositions[i].x()=left+int(maxs*minMoveStep)-(top-pinsPositions[i].y());
                               pinsPositions[i].y()=top-pinHeight/2;                             
                               //outerCircle <<"1111111111111111"<<endl;
                           }
                           //else, we just move it as far as we can
                           else {
                               maxs=maxmove;
                               pinsPositions[i].x()=left+int(maxs*minMoveStep)-(top-pinsPositions[i].y());
                               pinsPositions[i].y()=top-pinHeight/2;       
                               //outerCircle <<"----------------"<<pinsPositions[i].x()<<"---------------"<<pinsPositions[i].y()<<endl;
                           }
                       }
                       
                       //else we only move to top as much as we can
                       else {
                           maxs=maxmove;
                           pinsPositions[i].y() += int(maxs*minMoveStep);
                           //outerCircle <<"22222222222"<<endl;
                       }
                       
                       valid=0;
                       v1=0;
                       v2=0;
                       
                       //at this stage, we have moved the pin to the best position, but we don't know if it's legal yet                     
                       //while the pin assignment is not legal, update the position
                       while (valid==0){
                           for (int n=0;n<maxs;n=n+1){
                               for (int c=0;c<countPins;c=c+1){
                               
                                   //if pin assigned on left edge of macro
                                   if ((pinsPositions[c].x()==left+pinHeight/2)&&(pinsPositions[i].x()==left+pinHeight/2)&&(pinsPositions[i].y()<=invalidPositions[0][c])&&(pinsPositions[i].y()>=invalidPositions[1][c])){
                                       v1=v1+1;
                                                                                                                
                                   }
                                   //if pin assigned to top edge of macro
                                   else if ((pinsPositions[c].y()==top-pinHeight/2)&&(pinsPositions[i].y()==top-pinHeight/2)&&(pinsPositions[i].x()<=invalidPositions[0][c])&&(pinsPositions[i].x()>=invalidPositions[1][c])){
                                       v2=v2+1;                                                                           
                                   }
                               }
                               //outerCircle <<"v1---------"<<v1<<"----v2-------"<<v2<<endl;
                               //we are now in valid position
                               if ((v1==0)&&(v2==0)){
                                   valid=1;
                                   //if pin at top edge
                                   if(pinsPositions[i].y()+pinHeight/2 == top) {
                                          invalidPositions[0][i] = pinsPositions[i].x()+pinWidth + minPinPitch;
                                          invalidPositions[1][i] = pinsPositions[i].x()-pinWidth - minPinPitch;
                                   }
                                   else {
                                         invalidPositions[0][i] = pinsPositions[i].y()+pinHeight + minPinPitch;
                                         invalidPositions[1][i] = pinsPositions[i].y()-pinHeight - minPinPitch;
                                   }
                                   break;
                               }
                               else if ((v1>0)&&(v2==0)){
                                   pinsPositions[i].y()-=minMoveStep;
                                   v1=0;
                                   
                               }
                               else if ((v1==0)&&(v2>0)){
                                   if(pinsPositions[i].x() > left + pinHeight/2 + minPinPitch/2) {
                                         pinsPositions[i].x()= pinsPositions[i].x() - minMoveStep;
                                         v2=0;
                                   }
                                   else  {
                                       pinsPositions[i].y()=int((top- pinHeight/2 - minPinPitch/2-initialY)/minMoveStep)*minMoveStep+initialY;
                                       pinsPositions[i].x() = left + pinHeight/2;
                                       v2=0;
                                   } 
                               }
                           }
                       }
                     } //end of if pinEdge[i] == 4
                     
                     
                     //if the pin is at the bottom, we move right first and then top
                     if(pinEdge[i]==2) {
                             if  (float(pinsPositions[i].x()+maxmove*minMoveStep)>float(right-pinHeight/2-minPinPitch/2)){
                                 if (float(pinsPositions[i].y()+maxmove*minMoveStep-(right-pinsPositions[i].x()))>float(top-pinHeight/2-minPinPitch/2)){
                                     maxs=int(floor(float(top-pinHeight/2-minPinPitch/2-bottom+right-pinsPositions[i].x())/float(minMoveStep))); 
                                     pinsPositions[i].y()=bottom+int(maxs*minMoveStep)-(right-pinsPositions[i].x());
                                     pinsPositions[i].x()=right-pinWidth/2;
                                     
                                 }
                                 else {
                                     maxs=maxmove;
                                     pinsPositions[i].y() = bottom+int(maxs*minMoveStep)-(right-pinsPositions[i].x());
                                     pinsPositions[i].x()=right-pinWidth/2; 
                                     
                                 }
                             }
                             //otherwise, we move pin right as much as we can
                             else {
                                 maxs=maxmove;
                                 pinsPositions[i].x()+=int(maxs*minMoveStep);
                                 
                             }
                             
                             valid=0;
                             v1=0;
                             v2=0;
                             while (valid==0){
                                 for (int n=0;n<maxs;n=n+1){
                                     for (int c=0;c<countPins;c=c+1){
                                         if ((pinsPositions[c].y()==bottom+pinHeight/2)&&(pinsPositions[i].y()==bottom+pinHeight/2)&&(pinsPositions[i].x()<=invalidPositions[0][c])&&(pinsPositions[i].x()>=invalidPositions[1][c])){
                                             v1=v1+1;                                                                           
                                         }
                                         else if ((pinsPositions[c].x()==right-pinHeight/2)&&(pinsPositions[i].x()==right-pinHeight/2)&&(pinsPositions[i].y()<=invalidPositions[0][c])&&(pinsPositions[i].y()>=invalidPositions[1][c])){
                                             v2=v2+1;                                                                           
                                         }
                                     }
                                     
                                     //we reached a legal position
                                     if ((v1==0)&&(v2==0)){
                                         valid=1;
                                         //if pin final position at bottom edge
                                         if(pinsPositions[i].y()-pinHeight/2 == bottom) {
                                                invalidPositions[0][i] = pinsPositions[i].x()+pinWidth + minPinPitch;
                                                invalidPositions[1][i] = pinsPositions[i].x()-pinWidth - minPinPitch;
                                         }
                                         else {
                                               invalidPositions[0][i] = pinsPositions[i].y()+pinHeight + minPinPitch;
                                               invalidPositions[1][i] = pinsPositions[i].y()-pinHeight - minPinPitch;
                                         }
                                         break;
                                     }
                                     else if ((v1>0)&&(v2==0)){
                                         pinsPositions[i].x()-=minMoveStep;
                                         v1=0;
                                     }
                                     else if ((v1==0)&&(v2>0)){
                                         if(pinsPositions[i].y() > bottom + pinHeight/2 + minPinPitch/2) {
                                         pinsPositions[i].y()= pinsPositions[i].y()-minMoveStep;
                                         v2=0;
                                         }
                                         else {
                                             pinsPositions[i].x() = initialX + int((right-initialX - pinHeight/2 - minPinPitch/2)/minMoveStep)*minMoveStep;
                                             //outerCircle << "pinsPositions[i].x() 2: " << pinsPositions[i].x() << endl;
                                             pinsPositions[i].y() = bottom + pinHeight/2;
                                             v2=0;
                                         }
                                     }
                                 }
                             }
                     
                     } //end of if pinEdge[i] 
                     
                     
                 } //end of if pinDirections[i] ==13
                 
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------    
//-------------------------------------------------------------------------------------------------------------------------------------------                     
                 
                 //move pin to top-left-----------------
                 if (pinDirections[i]==14){
                     
                     //case 1: for pins on right edge, we move them top first and then left
                     if (pinEdge[i] == 3) {
                         
                         
                         if  (float(pinsPositions[i].y()+maxmove*minMoveStep)>float(top-pinHeight/2-minPinPitch/2)){
                         //if the max move goes beyond macro boundary, we limit it to top-left corner
                             if (float(pinsPositions[i].x()-(maxmove*minMoveStep-(top-pinsPositions[i].y())))<float(left+pinHeight/2+minPinPitch/2)){
                                 maxs=int(floor(float(right-pinHeight/2-minPinPitch/2-left+top-pinsPositions[i].y())/(minMoveStep)));
                                 pinsPositions[i].x()=right-(int(maxs*minMoveStep)-(top-pinsPositions[i].y()));
                                 pinsPositions[i].y()=top-pinHeight/2;
                                 valid=0;
                                 v1=0;
                                 v2=0;
                             }
                             //else, we just move it as far as we can
                             else {
                                 maxs=maxmove;
                                 pinsPositions[i].x()=right-(int(maxs*minMoveStep)-(top-pinsPositions[i].y()));
                                 pinsPositions[i].y()=top-pinHeight/2;
                                 valid=0;
                                 v1=0;
                                 v2=0;
                             }
                         }
                         
                         //else we only move to top as much as we can
                         else {
                             maxs=maxmove;
                             pinsPositions[i].y()=int(maxs*minMoveStep)+pinsPositions[i].y();
                             valid=0;
                             v1=0;
                             v2=0;
                         }
                         
                         //at this stage, we have moved the pin to the best position, but we don't know if it's legal yet
                           
                           //while the pin assignment is not legal, update the position
                         while (valid==0){
                             for (int n=0;n<maxs;n=n+1){
                                 for (int c=0;c<countPins;c=c+1){
                                 //if pin assigned on left edge of macro
                                     if ((pinsPositions[c].x()==right-pinHeight/2)&&(pinsPositions[i].x()==right-pinHeight/2)&&(pinsPositions[i].y()<=invalidPositions[0][c])&&(pinsPositions[i].y()>=invalidPositions[1][c])){
                                         v1=v1+1;                                                                           
                                     }
                                     else if ((pinsPositions[c].y()==top-pinHeight/2)&&(pinsPositions[i].y()==top-pinHeight/2)&&(pinsPositions[i].x()<=invalidPositions[0][c])&&(pinsPositions[i].x()>=invalidPositions[1][c])){
                                         v2=v2+1;
                                                                                                                
                                     }
                                 }
                                 //we are now in valid position
                                 if ((v1==0)&&(v2==0)){
                                     valid=1;
                                     //if pin at top edge
                                       if(pinsPositions[i].y()+pinHeight/2 == top) {
                                              invalidPositions[0][i] = pinsPositions[i].x()+pinWidth + minPinPitch;
                                              invalidPositions[1][i] = pinsPositions[i].x()-pinWidth - minPinPitch;
                                       }
                                       else {
                                             invalidPositions[0][i] = pinsPositions[i].y()+pinHeight + minPinPitch;
                                             invalidPositions[1][i] = pinsPositions[i].y()-pinHeight - minPinPitch;
                                       }
                                       break;
                                 }
                                 else if ((v1>0)&&(v2==0)){
                                     pinsPositions[i].y()=pinsPositions[i].y()-minMoveStep;
                                     v1=0;
                                 }
                                 else if ((v1==0)&&(v2>0)){
                                     if(pinsPositions[i].x()< right - pinHeight/2 - minPinPitch/2) {
                                         pinsPositions[i].x()= pinsPositions[i].x()+minMoveStep;
                                         v2=0;
                                     }
                                     else  {
                                         pinsPositions[i].y()=int((top- pinHeight/2 - minPinPitch/2-initialY)/minMoveStep)*minMoveStep+initialY;
                                         pinsPositions[i].x() = right - pinHeight/2;
                                         v2=0;
                                     }
                                     //else if (pinsPositions[i].y()==top - pinHeight/2){
                                     //    pinsPositions[i].y() -= minMoveStep;
                                     //    v2=0;
                                     //}
                                 }
                             }
                         }
                     }//end of if pinEdge[i] == 3
                     
                     
                     //if the pin is at the bottom, we move left first and then top
                     if (pinEdge[i]==2){
                         
                         if  (float(pinsPositions[i].x()-maxmove*minMoveStep)<float(left+pinHeight/2+minPinPitch/2)){
                             if (float(pinsPositions[i].y()+maxmove*minMoveStep-(pinsPositions[i].x()-left))>float(top-pinHeight/2-minPinPitch/2)){
                                 maxs=int(floor(float(top-pinHeight/2-minPinPitch/2-bottom+pinsPositions[i].x()-left)/(minMoveStep)));                 
                                 pinsPositions[i].y()=bottom+int(maxs*minMoveStep)-(pinsPositions[i].x()-left);
                                 pinsPositions[i].x()=left+pinWidth/2;
                                 valid=0;
                                 v1=0;
                                 v2=0;
                                 //innerCircle << "----------------"<<pinsPositions[i].x() << ", " << pinsPositions[i].y() << ";" << endl;
                             }
                             else {
                                 maxs=maxmove;
                                 pinsPositions[i].y()=bottom+int(maxs*minMoveStep)-(pinsPositions[i].x()-left);
                                 pinsPositions[i].x()=left+pinWidth/2;
                                 valid=0;
                                 v1=0;
                                 v2=0;
                             }
                         }
                         //otherwise, we move pin right as much as we can
                         else {
                             maxs=maxmove;
                             pinsPositions[i].x()=pinsPositions[i].x()-int(maxs*minMoveStep);
                             valid=0;
                             v1=0;
                             v2=0;
                         }
                         while (valid==0){
                             for (int n=0;n<maxs;n=n+1){
                                 for (int c=0;c<countPins;c=c+1){
                                     if ((pinsPositions[c].y()==bottom+pinHeight/2)&&(pinsPositions[i].y()==bottom+pinHeight/2)&&(pinsPositions[i].x()<=invalidPositions[0][c])&&(pinsPositions[i].x()>=invalidPositions[1][c])){
                                         v1=v1+1;                                                                           
                                     }
                                     else if ((pinsPositions[c].x()==left+pinHeight/2)&&(pinsPositions[i].x()==left+pinHeight/2)&&(pinsPositions[i].y()<=invalidPositions[0][c])&&(pinsPositions[i].y()>=invalidPositions[1][c])){
                                         v2=v2+1;                                                                           
                                     }
                                 }
                                 
                                 //we reached a legal position
                                 if ((v1==0)&&(v2==0)){
                                     valid=1;
                                     //if pin final position at bottom edge
                                     if(pinsPositions[i].y()-pinHeight/2 == bottom) {
                                                invalidPositions[0][i] = pinsPositions[i].x()+pinWidth + minPinPitch;
                                                invalidPositions[1][i] = pinsPositions[i].x()-pinWidth - minPinPitch;
                                         }
                                         else {
                                               invalidPositions[0][i] = pinsPositions[i].y()+pinHeight + minPinPitch;
                                               invalidPositions[1][i] = pinsPositions[i].y()-pinHeight - minPinPitch;
                                         }
                                     break;
                                 }
                                 else if ((v1>0)&&(v2==0)){
                                 
                                     pinsPositions[i].x()=pinsPositions[i].x()+minMoveStep;
                                     //outerCircle << "pinsPositions[i].x() 1: " << pinsPositions[i].x() << endl;
                                     
                                     v1=0;
                                 }
                                 else if ((v1==0)&&(v2>0)){
                                     //pinsPositions[i].y()=pinsPositions[i].y()-minMoveStep;
                                     if(pinsPositions[i].y() > bottom + pinHeight/2 + minPinPitch/2) {
                                         pinsPositions[i].y()= pinsPositions[i].y()-minMoveStep;
                                         v2=0;
                                     }
                                     else {
                                         pinsPositions[i].x() = initialX - int((initialX - left - pinHeight/2 - minPinPitch/2)/minMoveStep)*minMoveStep;
                                         //outerCircle << "pinsPositions[i].x() 2: " << pinsPositions[i].x() << endl;
                                         pinsPositions[i].y() = bottom + pinHeight/2;
                                         v2=0;
                                     }
                                     //else if (pinsPositions[i].x()==left + pinHeight/2){
                                     //    pinsPositions[i].x() += minMoveStep;
                                     //    v2=0;
                                     //}
                                 }
                             }
                         }
                     
                     
                     }//end of if pinEdge[i] == 2
                 }//end of if pinDirections[i] ==14
                 
                 
//-------------------------------------------------------------------------------------------------------------------------------------------    
//-------------------------------------------------------------------------------------------------------------------------------------------  
                 //move pin to bottom-right-----------------
                 if (pinDirections[i]==23){
                     
                     //case 1: for pins on left edge, we move them bottom first and then right
                     if (pinEdge[i] == 4) {
                         if  (float(pinsPositions[i].y()-maxmove*minMoveStep)<float(bottom+pinHeight/2+minPinPitch/2)){
                         //if the max move goes beyond macro boundary, we limit it to bottom-right corner
                             if (float(pinsPositions[i].x()+maxmove*minMoveStep-(pinsPositions[i].y()-bottom))>float(right-pinHeight/2-minPinPitch/2)){
                                 maxs=int(floor(float(right-pinHeight/2-minPinPitch/2-left+pinsPositions[i].y()-bottom)/float(minMoveStep)));
                                 pinsPositions[i].x()=left+int(maxs*minMoveStep)-(pinsPositions[i].y()-bottom);
                                 pinsPositions[i].y()=bottom+pinWidth/2;
                                 
                             }
                             //else, we just move it as far as we can
                             else {
                                 maxs=maxmove;
                                 pinsPositions[i].x()=left+int(maxs*minMoveStep)-(pinsPositions[i].y()-bottom);
                                 pinsPositions[i].y()=bottom+pinWidth/2;
                                 
                             }
                         }
                         //else we only move to top as much as we can
                         else {
                             maxs=maxmove;
                             pinsPositions[i].y()=pinsPositions[i].y()-int(maxs*minMoveStep);
                             
                         }
                         valid=0;
                         v1=0;
                         v2=0;
                         //at this stage, we have moved the pin to the best position, but we don't know if it's legal yet
                       
                       //while the pin assignment is not legal, update the position
                         while (valid==0){
                             for (int n=0;n<maxs;n=n+1){
                                 for (int c=0;c<countPins;c=c+1){
                                 
                                 //if pin assigned on left edge of macro
                                     if ((pinsPositions[c].x()==left+pinHeight/2)&&(pinsPositions[i].x()==left+pinHeight/2)&&(pinsPositions[i].y()<=invalidPositions[0][c])&&(pinsPositions[i].y()>=invalidPositions[1][c])){
                                         v1=v1+1;                                                                           
                                     }
                                     else if ((pinsPositions[c].y()==bottom+pinHeight/2)&&(pinsPositions[i].y()==bottom+pinHeight/2)&&(pinsPositions[i].x()<=invalidPositions[0][c])&&(pinsPositions[i].x()>=invalidPositions[1][c])){
                                         v2=v2+1;                                                                           
                                     }
                                 }
                                 
                                 //we are now in valid position
                                 if ((v1==0)&&(v2==0)){
                                     valid=1;
                                       //if pin at bottom edge
                                     if(pinsPositions[i].y()-pinHeight/2 == bottom) {
                                            invalidPositions[0][i] = pinsPositions[i].x()+pinWidth + minPinPitch;
                                            invalidPositions[1][i] = pinsPositions[i].x()-pinWidth - minPinPitch;
                                     }
                                     else {
                                           invalidPositions[0][i] = pinsPositions[i].y()+pinHeight + minPinPitch;
                                           invalidPositions[1][i] = pinsPositions[i].y()-pinHeight - minPinPitch;
                                     }
                                     break;
                                 }
                                 else if ((v1>0)&&(v2==0)){
                                     pinsPositions[i].y()=pinsPositions[i].y()+minMoveStep;
                                     v1=0;
                                 }
                                 else if ((v1==0)&&(v2>0)){
                                     if(pinsPositions[i].x()> left + pinHeight/2 + minPinPitch/2) {
                                         pinsPositions[i].x()= pinsPositions[i].x()-minMoveStep;
                                         v2=0;
                                         //innerCircle << "pinsPositions[i].x()---------: " << pinsPositions[i].x() << endl;
                                     }
                                     else  {
                                         pinsPositions[i].y()= initialY - int((initialY - bottom - pinHeight/2 - minPinPitch/2)/minMoveStep)*minMoveStep;
                                         pinsPositions[i].x() = left + pinHeight/2;
                                         v2=0;
                                     }
                                 }
                             }
                         }
                     }//end of if pinEdge[i] == 4
                     
                     //if the pin is at the top, we move right first and then bottom
                     if (pinEdge[i] == 1){
                         if  (float(pinsPositions[i].x()+maxmove*minMoveStep)>float(right-pinHeight/2-minPinPitch/2)){
                             if (float(pinsPositions[i].y()-(maxmove*minMoveStep-(right-pinsPositions[i].x())))<float(bottom+pinHeight/2+minPinPitch/2)){
                                 maxs=int(floor(float(top-pinHeight/2-minPinPitch/2-bottom+right-pinsPositions[i].x())/float(minMoveStep)));
                              
                                 pinsPositions[i].y()=top-(int(maxs*minMoveStep)-(right-pinsPositions[i].x()));
                                 pinsPositions[i].x()=right-pinWidth/2;
                                 
                             }
                             else {
                                 maxs=maxmove;
                                 pinsPositions[i].y()=top-(int(maxs*minMoveStep)-(right-pinsPositions[i].x()));
                                 pinsPositions[i].x()=right-pinWidth/2;
                                 
                             }
                         }
                         //otherwise, we move pin right as much as we can
                         else {
                             maxs=maxmove;
                             pinsPositions[i].x()=int(maxs*minMoveStep)+pinsPositions[i].x();
                             
                         }
                         valid=0;
                         v1=0;
                         v2=0;
                         while (valid==0){
                             for (int n=0;n<maxs;n=n+1){
                                 for (int c=0;c<countPins;c=c+1){
                                     if ((pinsPositions[c].y()==top-pinHeight/2)&&(pinsPositions[i].y()==top-pinHeight/2)&&(pinsPositions[i].x()<=invalidPositions[0][c])&&(pinsPositions[i].x()>=invalidPositions[1][c])){
                                         v1=v1+1;                                                                           
                                     }
                                     else if ((pinsPositions[c].x()==right-pinHeight/2)&&(pinsPositions[i].x()==right-pinHeight/2)&&(pinsPositions[i].y()<=invalidPositions[0][c])&&(pinsPositions[i].y()>=invalidPositions[1][c])){
                                         v2=v2+1;                                                                           
                                     }
                                 }
                                 //we reached a legal position
                                 if ((v1==0)&&(v2==0)){
                                     valid=1;
                                     //if pin final position at top edge
                                     if(pinsPositions[i].y()+pinHeight/2 == top) {
                                            invalidPositions[0][i] = pinsPositions[i].x()+pinWidth + minPinPitch;
                                            invalidPositions[1][i] = pinsPositions[i].x()-pinWidth - minPinPitch;
                                     }
                                     else {
                                           invalidPositions[0][i] = pinsPositions[i].y()+pinHeight + minPinPitch;
                                           invalidPositions[1][i] = pinsPositions[i].y()-pinHeight - minPinPitch;
                                     }
                                     break;
                                 }
                                 else if ((v1>0)&&(v2==0)){
                                     pinsPositions[i].x()=pinsPositions[i].x()-minMoveStep;
                                     v1=0;
                                 }
                                 else if ((v1==0)&&(v2>0)){
                                     if(pinsPositions[i].y() < top - pinHeight/2 - minPinPitch/2) {
                                         pinsPositions[i].y()= pinsPositions[i].y()+minMoveStep;
                                         v2=0;
                                     }
                                     //will only execute else once because we will go on top edge, so next iteration, v2 stays at 0 and v1 can get incremented
                                     else {
                                         pinsPositions[i].x() = initialX + int((right - pinHeight/2 - minPinPitch/2 - initialX)/minMoveStep)*minMoveStep;
                                         //outerCircle << "pinsPositions[i].x() 2: " << pinsPositions[i].x() << endl;
                                         pinsPositions[i].y() = top - pinHeight/2;
                                         v2=0;
                                     }
                                 }
                             }
                         }
                     }//end of if pinEdge[i] == 1
                 }//end of if pinDirections[i] ==23
                 
                 
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------    

//move pin to bottom-left-----------------
                 if (pinDirections[i]==24){
                     //case 1: for pins on left edge, we move them top first and then right
                     if (pinEdge[i] == 3) {
                         if  (float(pinsPositions[i].y()-maxmove*minMoveStep) < float(bBox.bottom()+pinHeight/2+minPinPitch/2)){
                         //if the max move goes beyond macro boundary, we limit it to bottom-left corner
                            
                            if (float(pinsPositions[i].x()-(maxmove*minMoveStep-(pinsPositions[i].y()-bottom)))<float(left+pinHeight/2+minPinPitch/2)){
                                 maxs=int(floor(float(right-pinHeight/2-minPinPitch/2-left+pinsPositions[i].y()-bottom)/float(minMoveStep)));
                                 pinsPositions[i].x()=right-(int(maxs*minMoveStep)-(pinsPositions[i].y()-bottom));
                                 pinsPositions[i].y()=bottom+pinWidth/2;
                                 
                             }
                             //else, we just move it as far as we can
                             else {
                                 maxs=maxmove;
                                 pinsPositions[i].x()=right-(int(maxs*minMoveStep)-(pinsPositions[i].y()-bottom));
                                 pinsPositions[i].y()=bottom+pinWidth/2;
                                 
                             }
                         }
                         //else we only move to top as much as we can
                         else {
                             maxs=maxmove;
                             pinsPositions[i].y()=pinsPositions[i].y()-int(maxs*minMoveStep);
                             
                         }
                         valid=0;
                         v1=0;
                         v2=0;
                         //at this stage, we have moved the pin to the best position, but we don't know if it's legal yet
                       
                       //while the pin assignment is not legal, update the position
                         while (valid==0){
                             for (int n=0;n<maxs;n=n+1){
                                 for (int c=0;c<countPins;c=c+1){
                                 //if pin assigned on left edge of macro
                                     if ((pinsPositions[c].x()==right-pinHeight/2)&&(pinsPositions[i].x()==right-pinHeight/2)&&(pinsPositions[i].y()<=invalidPositions[0][c])&&(pinsPositions[i].y()>=invalidPositions[1][c])){
                                         v1=v1+1;                                                                           
                                     }
                                     else if ((pinsPositions[c].y()==bottom+pinHeight/2)&&(pinsPositions[i].y()==bottom+pinHeight/2)&&(pinsPositions[i].x()<=invalidPositions[0][c])&&(pinsPositions[i].x()>=invalidPositions[1][c])){
                                         v2=v2+1;                                                                           
                                     }
                                     
                                 }
                                 //we are now in valid position
                                 if ((v1==0)&&(v2==0)){
                                     valid=1;
                                     //if pin at top edge
                                     if(pinsPositions[i].y()+pinHeight/2 == bottom) {
                                            invalidPositions[0][i] = pinsPositions[i].x()+pinWidth/2 + minPinPitch;
                                            invalidPositions[1][i] = pinsPositions[i].x()-pinWidth/2 - minPinPitch;
                                     }
                                     else {
                                           invalidPositions[0][i] = pinsPositions[i].y()+pinHeight/2 + minPinPitch;
                                           invalidPositions[1][i] = pinsPositions[i].y()-pinHeight/2 - minPinPitch;
                                     }
                                     break;
                                 }
                                 else if ((v1>0)&&(v2==0)){
                                     pinsPositions[i].y()=pinsPositions[i].y()+minMoveStep;
                                     v1=0;
                                 }
                                 else if ((v1==0)&&(v2>0)){
                                     if(pinsPositions[i].x()< right - pinHeight/2 - minPinPitch/2) {
                                         pinsPositions[i].x()= pinsPositions[i].x()+minMoveStep;
                                         v2=0;
                                     }
                                     else  {
                                         pinsPositions[i].y()= initialY - int((initialY - bottom - pinHeight/2 - minPinPitch/2)/minMoveStep)*minMoveStep;
                                         pinsPositions[i].x() = right - pinHeight/2;
                                         v2=0;
                                     }                                     
                                 }
                             }
                         
                         }
                     }//end of if pinEdge[i] == 3
                     
                     
                     //if the pin is at the bottom, we move right first and then top
                     if (pinEdge[i] == 1){
                         if  (float(pinsPositions[i].x()-maxmove*minMoveStep)<float(left+pinHeight/2+minPinPitch/2)){
                             if (float(pinsPositions[i].y()-(maxmove*minMoveStep-(pinsPositions[i].x()-left)))<float(bottom+pinHeight/2+minPinPitch/2)){
                                 maxs=int(floor(float(top-pinHeight/2-minPinPitch/2-bottom+pinsPositions[i].x()-left)/float(minMoveStep)));
                                 pinsPositions[i].y()=top-(int(maxs*minMoveStep)-(pinsPositions[i].x()-left));
                                 pinsPositions[i].x()=left+pinWidth/2;
                                 
                             }
                             else {
                                 maxs=maxmove;
                                 pinsPositions[i].y()=top-(int(maxs*minMoveStep)-(pinsPositions[i].x()-left));
                                 pinsPositions[i].x()=left+pinWidth/2;
                                 
                             }
                         }
                         //otherwise, we move pin right as much as we can
                         else {
                             maxs=maxmove;
                             pinsPositions[i].x()=pinsPositions[i].x()-int(maxs*minMoveStep);
                             
                         }
                         valid=0;
                         v1=0;
                         v2=0;
                         while (valid==0){
                             for (int n=0;n<maxs;n=n+1){
                                 for (int c=0;c<countPins;c=c+1){
                                     if ((pinsPositions[c].y()==top-pinHeight/2)&&(pinsPositions[i].y()==top-pinHeight/2)&&(pinsPositions[i].x()<=invalidPositions[0][c])&&(pinsPositions[i].x()>=invalidPositions[1][c])){
                                         v1=v1+1;                                                                           
                                     }
                                     else if ((pinsPositions[c].x()==left+pinHeight/2)&&(pinsPositions[i].x()==left+pinHeight/2)&&(pinsPositions[i].y()<=invalidPositions[0][c])&&(pinsPositions[i].y()>=invalidPositions[1][c])){
                                         v2=v2+1;                                                                           
                                     }
                                     
                                 }
                                 //innerCircle <<"i "<<i<< "  pinsPositions[i].x()---------: " << pinsPositions[i].x()<< "v1---------: " << v1<< "  v2---------: "<<v2<< endl;
                                 //we reached a legal position
                                 if ((v1==0)&&(v2==0)){
                                     valid=1;
                                     //if pin final position at bottom edge
                                     if(pinsPositions[i].y()-pinHeight/2 == top) {
                                            invalidPositions[0][i] = pinsPositions[i].x()+pinWidth/2 + minPinPitch;
                                            invalidPositions[1][i] = pinsPositions[i].x()-pinWidth/2 - minPinPitch;
                                     }
                                     else {
                                           invalidPositions[0][i] = pinsPositions[i].y()+pinHeight/2 + minPinPitch;
                                           invalidPositions[1][i] = pinsPositions[i].y()-pinHeight/2 - minPinPitch;
                                     }
                                     break;
                                 }
                                 else if ((v1>0)&&(v2==0)){
                                     pinsPositions[i].x()=pinsPositions[i].x()+minMoveStep;
                                     v1=0;
                                 }
                                 else if ((v1==0)&&(v2>0)){
                                     if(pinsPositions[i].y()< top - pinHeight/2 - minPinPitch/2) {
                                         pinsPositions[i].y()= pinsPositions[i].y()+minMoveStep;
                                         v2=0;
                                     }
                                     else  {
                                         pinsPositions[i].x()= initialX - int((initialX - left - pinHeight/2 - minPinPitch/2)/minMoveStep)*minMoveStep;
                                         pinsPositions[i].y() = top - pinHeight/2;
                                         v2=0;
                                     } 
                                     
                                 }
                             }
                         }
                     }//end of if pinEdge[i] 
                 }//end of if pinDirections[i] ==24
   */ 

             
             //outerCircle << pinsPositions[i].x() << ", " << pinsPositions[i].y() << ";" << endl;
             //innerCircle << "i "<<i<<" invalidPositions[0][i] " << invalidPositions[0][i] <<", invalidPositions[0][i] "<<invalidPositions[1][i]<< endl;   
             
//********************
//MOVE PINS
//********************
             
             OAHelper::MovePinToPosition(pinsInstTerm[i], pinsPositions[i]);
             
             
                                                                                                    
        } //end of for every pin



//FIND OTHER MACROS WITH SAME MASTER AND DO SAME PIN ASSIGNMENT
        i = 0;
        oaIter<oaInst>   masterIterator(block->getInsts());
        while (oaInst * master =  masterIterator.getNext()) {
                master->getCellName(ns, masterCellNameCurrent);
                master->getName(ns, instNameCurrent);
                
                masterName1 = masterName(masterCellNameCurrent, instNameCurrent);
                
                int indexInstName = masterCellNameCurrent.substr(instNameCurrent);

                
                
                //if the macro has same master as current macro, we do the same pin assignement
                //Note: we don't do anything in the case where we have the same macro
                if(masterName1==masterInstName && instNameCurrent!=instName) {
                      
                      oaOrient orientM = master->getOrient();
                      oaOrient orientOrignial = inst->getOrient();
                      
                      master->getBBox(bBoxSameMacro);      //returns in bBox, the bounding box of the macro
                      bBoxSameMacro.getCenter(centerSameMacro); 

                                                            
                      bool diffOrientation = orientM.getName()!=orientOrignial.getName(); 
                      
                      int angle = diffAngle(orientM.getName(), orientOrignial.getName());
                           
                          //for every pin of the current macro
                          oaIter<oaInstTerm> instTermIteratorS(master->getInstTerms());
                          while (oaInstTerm* instTermSameMacro = instTermIteratorS.getNext()) {
                          
                              instTermSameMacro->getTermName(ns, instTermSameMacroName);                  
                              //outerCircle << "instTermSameMacroName: " << instTermSameMacroName << endl;
                              
                              oaPoint pinMacroPos = OAHelper::GetAbsoluteInstTermPosition(instTermSameMacro);
                                 
                                 //outerCircle << "pin: " << pinMacroPos.x() << ", " << pinMacroPos.y() << ";" << endl;
                              

                                for(int l=0; l<countPins; l++) {
                                     //we will find pin with same name in original macro and give it same position
                                     //outerCircle << "-----------------------------pinsNames[l]: " << pinsNames[l] << endl;
                                     if(instTermSameMacroName==pinsNames[l]) {
                                     
                                             oaPoint newPos;
                                             
                                             
                                             if (angle==90) {
                                                 x = pinsPositions[l].y()-ptCenter.y();
                                                 y = pinsPositions[l].x()-ptCenter.x();
                                             
                                                 newPos = oaPoint(centerSameMacro.x() - x, centerSameMacro.y() + y);
                                             }
                                             
                                             
                                             else if (angle==180) {
                                                 x = pinsPositions[l].x()-ptCenter.x();
                                                 y = pinsPositions[l].y()-ptCenter.y();
                                             
                                                 newPos = oaPoint(centerSameMacro.x() - x, centerSameMacro.y() - y);
                                             
                                             }
                                             else if(angle==270) {
                                             
                                                 x = pinsPositions[l].y()-ptCenter.y();
                                                 y = pinsPositions[l].x()-ptCenter.x();
                                             
                                                 newPos = oaPoint(centerSameMacro.x() + x, centerSameMacro.y() - y);
                                             
                                             }
                                            
                                             else {
                                             
                                             
                                                 x = pinsPositions[l].x()-ptCenter.x();
                                                 y = pinsPositions[l].y()-ptCenter.y();
                                             
                                                 newPos = oaPoint(centerSameMacro.x() + x, centerSameMacro.y() + y);
                                                   
                                             }
                                             
                                             OAHelper::MovePinToPosition(instTermSameMacro, newPos);
                                             
                                             //outerCircle << "newPos: " << newPos.x() << ", " << newPos.y() << ";" << endl;
                                             
                                             break;
                                             //i++; 
                                      }  
                                } //end of for
      
                              
                      }//end of while for every pin of current macro
                      
                                            
                      
                      
                      macroMasters[m] = masterName1;
                      m++;
                      
                      
                      
                }//end of if: macro is of the same master
                
                
        }//end of while: for every other macro (master)
   
       
       
   } //end of else: first time macro pin assignement

 
 
 } //end of while: for every macro of design
 


	//=====================================================================
	
    //Save the improved version of the design
    InputOutputHandler::SaveAndCloseAllDesigns(designInfo, design, block);

	if (lib)
		lib->close();

    cout << endl << "\nDone!" << endl;
    return 0;
}
