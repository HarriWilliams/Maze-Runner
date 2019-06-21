#include "allcode_api.h"
/*
This code contains four Finite State Machines (FSM), which gives the robot
several distinct behaviors that will allow it to navigate a maze, correct itself,
search for objectives within said maze and create a map of the maze itself.
*/

int main();

//Exploration FSM and its respective functions
void exploration();
int find_gaps();

//Correction FSM and its respective functions
void correction();
int orientation();

//Objective FSM and its respective functions
void objective();
int nest_detection();
int victory();

//Mapping FSM and its respective functions
void mapping();
void record_cell();
void update_orientation(int heading);
void update_coordinates();

//An array to hold each IR sensor value when they are at 2 - 3cm away from maze wall
const int IR_VALUES[8] = {700, 600, 680, 480, 450, 530, 1000, 420};
int is_next_cell = 0;

int cells_explored = 0;

//Variables to calculate which wall is closer: the left, right or front wall
#define left ((FA_ReadIR(0) / IR_VALUES[0]) + (FA_ReadIR(1) / IR_VALUES[1]) + (FA_ReadIR(7) / IR_VALUES[7]))
#define right ((FA_ReadIR(3) / IR_VALUES[3]) + (FA_ReadIR(4) / IR_VALUES[4]) + (FA_ReadIR(5) / IR_VALUES[5]))
#define front ((FA_ReadIR(1) / IR_VALUES[1]) + (FA_ReadIR(2) / IR_VALUES[2]) + (FA_ReadIR(3) / IR_VALUES[3]))

//9x9 array to hold map data and x,y coordinates
//Map array is 9x9 to account for cell border, cells and walls between cells
int map [9][9] = {
	{1, 1, 1, 1, 1, 1, 1, 1, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 1, 1, 1, 1, 1, 1, 1, 1},
};
int x = 0;
int y = 0;

//1 = North, 2 = East, 3 = South, 4 = West
int direction;

//1 = left, 2 = right, 3 = down
const int heading_left = 1;
const int heading_right = 2;
const int heading_down = 3;

/*
Variables that control their respective FSMs (mapping_mode is initialized 
during setup process of main function)
*/
int exploration_mode = 1;
int correction_mode = 1;
int objective_mode = 1;
int mapping_mode = 0;
//
//-------------------------- FSM 1 - Exploration -------------------------------
//Determines which path the robot should take in the maze
//
void exploration() {
	switch (exploration_mode) {
		
		case 1:
		exploration_mode = find_gaps();
		break;
		
		case 2:
		//Gap is on left
		FA_Left(90);
		update_orientation(heading_left);
		exploration_mode = 6;
		break;
			
		case 3:
		//Gap is in front of robot, no need to use update_orientation
		exploration_mode = 6;
		break;
			
		case 4:
		//Gap is on right
		FA_Right(90);
		update_orientation(heading_right);
		exploration_mode =6;
		break;
		
		case 5:
		//There is no gap, robot will turn 180 degrees
		FA_Left(180);
		update_orientation(heading_down);
		exploration_mode = 6;
		break;
		
		case 6:
		FA_Forwards(150);
		exploration_mode = 1;
		break;
	}
}

int find_gaps() {
	int gap;
	
	if (FA_ReadIR(0) < 50){
		gap = 2;} 
	else if (FA_ReadIR(2) < 50) {
		gap = 3;} 
	else if (FA_ReadIR(4) < 50) {
		gap = 4;} 
	else { gap = 5; }
	
	return gap;
	
//Uses IR sensors and follows the left wall to find the appropriate gap to maneuver through
}
//
//-------------------------- FSM 2 - Correction -------------------------------
//Corrects robot's orientation using walls surrounding robot as reference
//
void correction() {
	switch (correction_mode) {
		
		case 1:
		//If the robot needs to be orientated, either case 2, 3 or 4 will activate
		correction_mode = orientation();
		break;
		
		case 2:
		//Robot is oriented towards wall on left side
		do {
			FA_Right(2);
		} while ((FA_ReadIR(0) / IR_VALUES[0]) < (FA_ReadIR(7) / IR_VALUES[7]));
		correction_mode = 1;
		break;
		
		case 3:
		//Robot is orientated towards wall on right side
		do {
			FA_Left(2);
		} while ((FA_ReadIR(3) / IR_VALUES[3]) < (FA_ReadIR(5) / IR_VALUES[5]));
		correction_mode = 1;
		break;
	}
}

int orientation() {
	int wall_side = 1;
	
	if (left > 2){
		wall_side = 2;}
	else if (right > 2) {
		wall_side = 3;}
		
	return wall_side;
	
//Detects if robot is too close to a wall or not
}
//
//-------------------------- FSM 3 - Objective -------------------------------
//Searches for the nest and whether the maze has been completed or not
//
void objective() {
	switch (objective_mode) {
		case 1:
		//Continually loops until nest has been found
		objective_mode = nest_detection();
		break;
		
		case 2:
		//Plays note to show that nest has been found
		objective_mode = 3;
		break;
		
		case 3:
		//Continually loops until the maze has been completed
		objective_mode = victory();
		break;
		
		case 4:
		//When maze has been completed, robot will celebrate
		FA_PlayNote(1200, 200);
		objective_mode = 5;
		break;
		
		case 5:
		/*
		This is an infinite loop, the robot has finished the maze 
		so there is no need for this FSM anymore.
		Simply here to prevent robot from playing the same note over 
		and over again because that's annoying
		*/
		break;
	}
}

int nest_detection() {
	int success = 1;
	
	if (FA_ReadLine(0) < 350 && FA_ReadLine(1) < 290 && FA_ReadLight < 100) {
		success = 2; }
	return success;
//Detects the objective (i.e. the nest) within the maze
}

int victory() {
	int total;
	int temp_total;
	int completed = 3;
	int x;
	int y;
	
	for (x = 0; x < 9; x = x + 1) {
		for (y = 0; y < 9; y = y + 1) {
			temp_total = map[x][y];
			total = total + temp_total; } 
			}
	if (total >= 41) {
		completed = 4; }
	return completed;
/*
There are 41 walls in the maze including the border wall. Since the mapping FSM 
marks a wall with the value of 1 in the map array, when the total value in the
map array is 41 that means that the maze has been fully explored.
*/
}
//
//-------------------------- FSM 4 - Mapping -------------------------------
//Maps the maze, tracks the robot's orientation and prints map of maze
//
void mapping() {
	switch (mapping_mode) {
		case 1:
		//Maze is not mirrored, starts close to bottom left corner
		x = 1;
		y = 5;
		mapping_mode = 3;
		break;
		
		case 2:
		//Maze is mirrored, starts close to bottom right corner
		x = 1;
		y = 3;
		mapping_mode = 3;
		break;
		
		case 3:
		//Records cell Robot is currently in, will remain in this switch statement for the remainder of run time
		record_cell();
		break;
	}
//Coordinates are: even number = cell, odd number = wall or gap between cells
}

void record_cell() {
	
	//Robot is facing North
	if (direction == 1) {
		
		if (FA_ReadIR(0) > 100) {
		map[x + 1][y] = 1;}
		else {map[x + 1][y] = 0;}
		
		if (FA_ReadIR(2) > 100) {
		map[x][y + 1] = 1;}
		else {map[x][y + 1] = 0;}
		
		if (FA_ReadIR(4) > 100) {
		map[x - 1][y] = 1;}
		else {map[x - 1][y] = 0;}
		
		if (FA_ReadIR(6) > 100) {
		map[x][y - 1] = 1;}
		else {map[x][y - 1] = 0;}
	}
	//Robot is facing East
	if (direction == 2) {
		
		if (FA_ReadIR(6) > 100) {
		map[x + 1][y] = 1;}
		else {map[x + 1][y] = 0;}
		
		if (FA_ReadIR(0) > 100) {
		map[x][y + 1] = 1;}
		else {map[x][y + 1] = 0;}
		
		if (FA_ReadIR(2) > 100) {
		map[x - 1][y] = 1;}
		else {map[x - 1][y] = 0;}
		
		if (FA_ReadIR(4) > 100) {
		map[x][y - 1] = 1;}
		else {map[x][y - 1] = 0;}
	}
	//Robot is facing South
	if (direction == 3) {
		
		if (FA_ReadIR(4) > 100) {
		map[x + 1][y] = 1;}
		else {map[x + 1][y] = 0;}
		
		if (FA_ReadIR(6) > 100) {
		map[x][y + 1] = 1;}
		else {map[x][y + 1] = 0;}
		
		if (FA_ReadIR(0) > 100) {
		map[x - 1][y] = 1;}
		else {map[x - 1][y] = 0;}
		
		if (FA_ReadIR(2) > 100) {
		map[x][y - 1] = 1;}
		else {map[x][y - 1] = 0;}
	}
	//Robot is facing West
	if (direction == 4) {
		
		if (FA_ReadIR(2) > 100) {
		map[x + 1][y] = 1;}
		else {map[x + 1][y] = 0;}
		
		if (FA_ReadIR(4) > 100) {
		map[x][y + 1] = 1;}
		else {map[x][y + 1] = 0;}
		
		if (FA_ReadIR(6) > 100) {
		map[x - 1][y] = 1;}
		else {map[x - 1][y] = 0;}
		
		if (FA_ReadIR(0) > 100) {
		map[x][y - 1] = 1;}
		else {map[x][y - 1] = 0;}
	}
//Records layout of cell using IR sensors
}

void update_orientation(int heading) {
//Variable heading: 1 = Moving left, 2 = Moving right, 3 = Moving down

	//Robot is currently facing North
	if ((direction == 1) && (heading == 1)) {
		direction = 4; }
	if ((direction == 1) && (heading == 2)) {
		direction = 2; }
	if ((direction == 1) && (heading == 3)) {
		direction = 3; }
	//Robot is currently facing East
	if ((direction == 2) && (heading == 1)) {
		direction = 1; }
	if ((direction == 2) && (heading == 2)) {
		direction = 3; }
	if ((direction == 2) && (heading == 3)) {
		direction = 4; }
	//Robot is currently facing South
	if ((direction == 3) && (heading == 1)) {
		direction = 2; }
	if ((direction == 3) && (heading == 2)) {
		direction = 4; }
	if ((direction == 3) && (heading == 3)) {
		direction = 1; }
	//Robot is currently facing West
	if ((direction == 4) && (heading == 1)) {
		direction = 3; }
	if ((direction == 4) && (heading == 2)) {
		direction = 1; }
	if ((direction == 4) && (heading == 3)) {
		direction = 2; }
//Changes orientation of robot depending on decision made by Exploration FSM
}

void update_coordinates() {
	
	//Robot is facing North
	if (direction == 1) {
		y = y + 2; }
	//Robot is facing East
	if (direction == 2) {
		x = x - 2; }
	//Robot is facing South
	if (direction == 3) {
		y = y - 2; }
	//Robot is facing West
	if (direction == 4) {
		x = x + 2; }
//Robot is moving forward one cell and changing one of its coordinates by 2
}
//
//------------------- Main Function --------------
//
int main() {
//Initialize the robot and sets the direction of the robot to North
	FA_RobotInit();
	FA_LCDBacklight(50);
	FA_LCDClear();
	
	direction = 1;
	FA_LCDPrint("Is the maze mirrored?", 5, 20, 25,FONT_NORMAL, LCD_OPAQUE);
	FA_LCDPrint("Left button = YES, Right Button = NO", 5, 40, 25,FONT_NORMAL, LCD_OPAQUE);
	
	if (FA_ReadSwitch(0) == 1) {
		mapping_mode = 1; }
	if (FA_ReadSwitch(1) == 1) {
		mapping_mode = 2; }
	FA_LCDClear();
	//Asks user for layout of maze and changes mapping FSM accordingly
	
	while(1)
	{
		exploration();
		correction();
		objective();
		mapping();
}
}