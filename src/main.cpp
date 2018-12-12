/*Connor Steele - Program 2 - CSC 474 Spring 2018*/


/*
Utilzed Tyler's base code from Piazza
musicvisualizer base code
by Christian Eckhardt 2018
with some code snippets from Ian Thomas Dunn and Zoe Wood, based on their CPE CSC 471 base code
On Windows, it whould capture "what you here" automatically, as long as you have the Stereo Mix turned on!! (Recording devices -> activate)
*/

// Core libraries
#include <cmath>
//3x below are for files
#include <iostream>
#include <iomanip>
#include <fstream>

// Third party libraries
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <time.h>

// Local headers
#include "GLSL.h"
#include "Program.h"
#include "WindowManager.h"
#include "Shape.h"
#include "Camera.h"
#include "tiny_obj_loader.h"

//timeing stuff
#include <ctime>
#include <ratio>
#include <chrono>

using namespace std;
using namespace glm;
using namespace std::chrono;

std::shared_ptr<Program> phongShader; //changed to global
high_resolution_clock::time_point t1, t2; //use global timers
int charindex; //global charater index
std::vector<char> txtarray;
std::vector<vector<char>> opFaceInput; // Array containing information from OpenFace output
std::vector<vector<vector<char>>> delimInput;
float interpol = 0.0; //goes from 0 to 1, send to shader as t

// Values used to animate the face
vector<double> upperLipAvg, lowerLipAvg, upperLeftEyeAvg, upperRightEyeAvg, lowerLeftEyeAvg, lowerRightEyeAvg;

double get_last_elapsed_time() {
	static double lasttime = glfwGetTime();
	double actualtime = glfwGetTime();
	double difference = actualtime - lasttime;
	lasttime = actualtime;
	return difference;
}

//
double frametime;
double gametime = 0; //initial is at 0
double facetime = 0;

class Application : public EventCallbacks {
public:
	WindowManager *windowManager = nullptr;
	Camera *camera = nullptr;


	//std::shared_ptr<Program> phongShader; //changed to global
	vector<tinyobj::shape_t> faceDefault, faceUpperLip100, faceUpperLip50; //For New Faces
	vector<tinyobj::material_t> jFaceMat, faceUpperLip100Mat, faceUpperLip50Mat; //For New Faces

	string curItem;

	bool mousePressed = false;
	bool mouseCaptured = false;
	glm::vec2 mouseMoveOrigin = glm::vec2(0);
	glm::vec3 mouseMoveInitialCameraRot;

	GLuint VertexArrayID, VAO;
	GLuint VertexBufferID1, VertexBufferID2, VertexBufferID3, VertexBufferID4, VertexBufferID5, VertexBufferID6, VertexBufferID7, VertexBufferID8, VertexBufferID9;
	GLuint VertexNormsID1, VertexNormsID2, VertexNormsID3, VertexNormsID4, VertexNormsID5, VertexNormsID6, VertexNormsID7, VertexNormsID8, VertexNormsID9;
	// Vertex Buffer and Norms for New Faces
	GLuint jFaceBuffDefault, jFaceNormsDefault, upperLip100Buff, upperLip100Norms, upperLip50Buff, upperLip50Norms;

	Application() {
		camera = new Camera();
	}

	~Application() {
		delete camera;
	}

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
		// Movement
		if (key == GLFW_KEY_W && action != GLFW_REPEAT) camera->vel.z = (action == GLFW_PRESS) * -0.2f;
		if (key == GLFW_KEY_S && action != GLFW_REPEAT) camera->vel.z = (action == GLFW_PRESS) * 0.2f;
		if (key == GLFW_KEY_A && action != GLFW_REPEAT) camera->vel.x = (action == GLFW_PRESS) * -0.2f;
		if (key == GLFW_KEY_D && action != GLFW_REPEAT) camera->vel.x = (action == GLFW_PRESS) * 0.2f;
		// Rotation
		if (key == GLFW_KEY_I && action != GLFW_REPEAT) camera->rotVel.x = (action == GLFW_PRESS) * 0.02f;
		if (key == GLFW_KEY_K && action != GLFW_REPEAT) camera->rotVel.x = (action == GLFW_PRESS) * -0.02f;
		if (key == GLFW_KEY_J && action != GLFW_REPEAT) camera->rotVel.y = (action == GLFW_PRESS) * 0.02f;
		if (key == GLFW_KEY_L && action != GLFW_REPEAT) camera->rotVel.y = (action == GLFW_PRESS) * -0.02f;
		if (key == GLFW_KEY_U && action != GLFW_REPEAT) camera->rotVel.z = (action == GLFW_PRESS) * 0.02f;
		if (key == GLFW_KEY_O && action != GLFW_REPEAT) camera->rotVel.z = (action == GLFW_PRESS) * -0.02f;
		// Disable cursor (allows unlimited scrolling)
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
			mouseCaptured = !mouseCaptured;
			glfwSetInputMode(window, GLFW_CURSOR, mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
			resetMouseMoveInitialValues(window);
		}
		//Mine, reset charindex
		if (key == GLFW_KEY_R && action == GLFW_PRESS)
		{
			charindex = 1;
			cout << "\n" << txtarray[0]; ;
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods) {
		mousePressed = (action != GLFW_RELEASE);
		if (action == GLFW_PRESS) {
			resetMouseMoveInitialValues(window);
		}
	}

	void mouseMoveCallback(GLFWwindow *window, double xpos, double ypos) {
		if (mousePressed || mouseCaptured) {
			float yAngle = (xpos - mouseMoveOrigin.x) / windowManager->getWidth() * 3.14159f;
			float xAngle = (ypos - mouseMoveOrigin.y) / windowManager->getHeight() * 3.14159f;
			camera->setRotation(mouseMoveInitialCameraRot + glm::vec3(-xAngle, -yAngle, 0));
		}
		//cout << "X: " << xpos / windowManager->getWidth() << "\n";
		//cout << "Y: " << ypos / windowManager->getHeight() << "\n";
	}

	void resizeCallback(GLFWwindow *window, int in_width, int in_height) { }

	// Reset mouse move initial position and rotation
	void resetMouseMoveInitialValues(GLFWwindow *window) {
		double mouseX, mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);
		mouseMoveOrigin = glm::vec2(mouseX, mouseY);
		mouseMoveInitialCameraRot = camera->rot;
	}

	void initGeom(const std::string& resourceDirectory) {

		string err;
		bool fbool;
		// Get Face Models
		fbool = false;
		fbool = tinyobj::LoadObj(faceDefault, jFaceMat, err, "../resources/faces/defaultFace.obj");
		if (faceDefault.size() <= 0) {
			cout << "jFaceDefault size < 0";
			return;
		}

		// Get Face Models
		fbool = false;
		fbool = tinyobj::LoadObj(faceUpperLip100, faceUpperLip100Mat, err, "../resources/faces/upperLip100Up.obj");
		if (faceUpperLip100.size() <= 0) {
			cout << "jFaceDefault size < 0";
			return;
		}

		// Get Face Models
		fbool = false;
		fbool = tinyobj::LoadObj(faceUpperLip50, faceUpperLip50Mat, err, "../resources/faces/upperLip50Up.obj");
		if (faceUpperLip50.size() <= 0) {
			cout << "jFaceDefault size < 0";
			return;
		}

		//BIND BUFFERS
		glGenVertexArrays(1, &VertexArrayID);
		glBindVertexArray(VertexArrayID);


		// Bind Default Face
		//Bind Default Face Verts
		glGenBuffers(1, &jFaceBuffDefault);
		glBindBuffer(GL_ARRAY_BUFFER, jFaceBuffDefault);
		glBufferData(GL_ARRAY_BUFFER, faceDefault[0].mesh.positions.size() * sizeof(float), &faceDefault[0].mesh.positions[0], GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); // layout(location = 0)
		//Bind Default FaceNorms
		glGenBuffers(1, &jFaceNormsDefault);
		glBindBuffer(GL_ARRAY_BUFFER, jFaceNormsDefault);
		glBufferData(GL_ARRAY_BUFFER, faceDefault[0].mesh.normals.size() * sizeof(float), &faceDefault[0].mesh.normals[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); // layout(location = 1)

		//Bind Face Verts
		glGenBuffers(1, &upperLip100Buff);
		glBindBuffer(GL_ARRAY_BUFFER, upperLip100Buff);
		glBufferData(GL_ARRAY_BUFFER, faceUpperLip100[0].mesh.positions.size() * sizeof(float), &faceUpperLip100[0].mesh.positions[0], GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); // layout(location = 2)
		//Bind FaceNorms
		glGenBuffers(1, &upperLip100Norms);
		glBindBuffer(GL_ARRAY_BUFFER, upperLip100Norms);
		glBufferData(GL_ARRAY_BUFFER, faceUpperLip100[0].mesh.normals.size() * sizeof(float), &faceUpperLip100[0].mesh.normals[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); // layout(location = 3)

		//Bind Face Verts
		glGenBuffers(1, &upperLip50Buff);
		glBindBuffer(GL_ARRAY_BUFFER, upperLip50Buff);
		glBufferData(GL_ARRAY_BUFFER, faceUpperLip50[0].mesh.positions.size() * sizeof(float), &faceUpperLip50[0].mesh.positions[0], GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		//Bind FaceNorms
		glGenBuffers(1, &upperLip50Norms);
		glBindBuffer(GL_ARRAY_BUFFER, upperLip50Norms);
		glBufferData(GL_ARRAY_BUFFER, faceUpperLip50[0].mesh.normals.size() * sizeof(float), &faceUpperLip50[0].mesh.normals[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	}

	void init(const std::string& resourceDirectory) {
		GLSL::checkVersion();

		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		// Initialize the GLSL programs
		phongShader = std::make_shared<Program>();
		phongShader->setShaderNames(resourceDirectory + "/phong.vert", resourceDirectory + "/phong.frag");
		phongShader->init();
		phongShader->addAttribute("vertPos1"); // layout(location = 0)
		phongShader->addAttribute("vertNor1"); // layout(location = 1)

		phongShader->addAttribute("vertPos2");
		phongShader->addAttribute("vertNor2");

		phongShader->addAttribute("vertPos3");
		phongShader->addAttribute("vertNor3");

		phongShader->addAttribute("vertPos4");
		phongShader->addAttribute("vertNor4");

		phongShader->addAttribute("vertPos5"); // layout(location = 9)
		phongShader->addAttribute("vertNor5");
		/*phongShader->addAttribute("vertPos6");
		phongShader->addAttribute("vertNor6");
		phongShader->addAttribute("vertPos7");
		phongShader->addAttribute("vertNor7");
		phongShader->addAttribute("vertPos8");
		phongShader->addAttribute("vertNor8");*/
		//phongShader->addAttribute("vertPos9");
		//phongShader->addAttribute("vertNor9");
		phongShader->addUniform("t");
		//my uniform for interpolation between faces
		phongShader->addUniform("facet");
		phongShader->addUniform("facetPrev");
	}

	glm::mat4 getPerspectiveMatrix() {
		float fov = 3.14159f / 4.0f;
		float aspect = windowManager->getAspect();
		return glm::perspective(fov, aspect, 0.01f, 10000.0f);
	}

	//My function to update the face, called in render(), add a button to reset charindex so you can restart the animation
	void updateFace()
	{
		//FACE KEY: Face AAh is 1, Face E is 2, Face FV is 3, Face LD is 4, Face O is 5, face slient/m/b is 6, face STCh is 7, face UR is 8
		static int count = 0;
		char ch, chprev;
		int ms = 40; //number of miliseconds to read one character, around 80 looks the best, around 40 for human like speed, use 1000 for debug
					 //Eckhardt said that 80 is fine for testing speed.

		//Number that relates to what faces to use
		//int face1 = 6, face2 = 6; //default to silent face, //Maybe get the initial face
		// Face 7 is new face for now
		static int face1 = 1, face2 = 1;


		t2 = high_resolution_clock::now(); //current time timer
		duration<double, std::milli> time_span = t2 - t1;
		interpol = time_span.count() / ms; //Get the interpolation amount based on the time interval
		if (interpol < 0.02) //stay in range 0.0 to 1.0
		{
			interpol = 0.0; //Stay at the old face 100%
		}
		if (interpol > 1.0) //stay in range 0.0 to 1.0
		{
			interpol = 1.0; //Go to the next face 100%
		}
		glUniform1f(phongShader->getUniform("t"), interpol); //send the interpolation amount to the shader
		//cout << "interpolation is: " << interpol << "\n"; //print interpolation float to console 


		if (count == 0)
		{
			cout << txtarray[0]; //print off the first letter of the input string only once
		}
		count += 1;
		//cout << "face1 is: " << face1 << " face2 is :" << face2 << "\n" ;
		if ((charindex < txtarray.size() - 1) && (time_span.count() >= ms)) //count() give time in milliseconds, so read a char off every 100ms
		{
			t1 = high_resolution_clock::now(); //update t1 to move to the next animation interval

			ch = txtarray.at(charindex); //get current char
			chprev = txtarray.at(charindex - 1); //get previous character


			face1 = face2;//Previous iteration current face should be face1
			glUniform1i(phongShader->getUniform("facetPrev"), face1); //send the previous face to the shader


			//All letters of the alphabet have been implemented
			//if (ch == 'A' || ch == 'a' || ch == 'H' || ch == 'h')
			//{
			//	face2 = 1;
			//	glUniform1i(phongShader->getUniform("facet"), face2); //send new face to the shader
			//}
			//else if (ch == 'E' || ch == 'e' || ch == 'I' || ch == 'i')
			//{
			//	face2 = 1;
			//	glUniform1i(phongShader->getUniform("facet"), face2); //send new face to the shader
			//}
			//else if (ch == 'F' || ch == 'f' || ch == 'V' || ch == 'v' || ch == 'Z' || ch == 'z')
			//{
			//	face2 = 1;
			//	glUniform1i(phongShader->getUniform("facet"), face2); //send new face to the shader
			//}
			//else if (ch == 'L' || ch == 'l' || ch == 'D' || ch == 'd' || ch == 'N' || ch == 'n' || ch == 'Y' || ch == 'y')
			//{
			//	face2 = 1;
			//	glUniform1i(phongShader->getUniform("facet"), face2); //send new face to the shader
			//}
			//else if (ch == 'O' || ch == 'o' || ch == 'W' || ch == 'w')
			//{
			//	face2 = 1;
			//	glUniform1i(phongShader->getUniform("facet"), face2); //send new face to the shader
			//}
			//else if (ch == ' '|| ch == 'M' || ch == 'm' || ch == 'B' || ch == 'b' || ch == 'P' || ch == 'p')
			//{
			//	face2 = 1;
			//	glUniform1i(phongShader->getUniform("facet"), face2); //send new face to the shader
			//}
			//else if (ch == 'S' || ch == 's' || ch == 'T' || ch == 't' || ch == 'C' || ch == 'c' 
			//		  || ch == 'K' || ch == 'k' || ch == 'Q' || ch == 'q' || ch == 'X' || ch == 'x')
			//{
			//	face2 = 1;
			//	glUniform1i(phongShader->getUniform("facet"), face2); //send new face to the shader
			//}
			//else if (ch == 'U' || ch == 'u' || ch == 'R' || ch == 'r' || ch == 'G' || ch == 'g'
			//		 || ch == 'J' || ch == 'j')
			//{
			//	face2 = 1;
			//	glUniform1i(phongShader->getUniform("facet"), face2); //send new face to the shader
			//}

			cout << txtarray[charindex]; //Output the current letter being read
			//cout << txtarray[charindex - 1] << txtarray[charindex]; //output what the current and prev letter is

			charindex += 1; //move onto the next character in the array
		}
		else if (charindex >= txtarray.size())
		{
			//cout << "\nFINISHED READING\n";
		}
	}

	void render() {
		frametime = get_last_elapsed_time();
		gametime += frametime;

		// Clear framebuffer.
		glClearColor(0.3f, 0.7f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Create the matrix stacks.
		glm::mat4 V, M, P;
		P = getPerspectiveMatrix();
		V = camera->getViewMatrix();
		M = glm::mat4(1);

		/**************/
		/* DRAW SHAPE */
		/**************/

		//Draw the face
		M = glm::translate(glm::mat4(1), glm::vec3(0, -1.0, -10));
		phongShader->bind();
		phongShader->setMVP(&M[0][0], &V[0][0], &P[0][0]);
		//shape->draw(phongShader, false);  //Draw the face_AAh.obj w/o going thru verts, DONT USE for final only use to check
		glBindVertexArray(VertexArrayID);
		glDrawArrays(GL_TRIANGLES, 0, faceDefault[0].mesh.positions.size() / 3); //Go thru the vertices of the face and draw them
		updateFace(); //bind the correct face to the shader based on input string
		phongShader->unbind(); //Finish

	}
};

// Read .txt file input
std::vector<char> getTxtFile()
{
	ifstream inFile;
	std::vector<char> txtarray; //store a char array of all characters in the file
	std::string resourceDir = "../resources";
	inFile.open(resourceDir + "/textInput.txt");
	if (!inFile) {
		cout << "Unable to open file";
		exit(1); // terminate with error
	}
	//Read off to an array
	char c;
	while (inFile.get(c))
	{
		txtarray.push_back(c);
	}

	inFile.close(); //Close the text file

	return txtarray;

}

std::vector<vector<char>> readOpenFaceOutput()
{
	ifstream inFile;
	//std::vector<string> txtarray; //store a char array of all characters in the file
	std::vector<vector<char>> strings;
	std::string resourceDir = "../resources";
	inFile.open(resourceDir + "/pixel2_selfie_cam_horizontal.csv");
	if (!inFile) {
		cout << "Unable to open file";
		exit(1); // terminate with error
	}

	//Read off to an array
	char c;
	std::vector<char> line;
	while (inFile.get(c))
	{
		if (c == '\n' || c == '\r\n')
		{
			strings.push_back(line);
			line.clear(); // reset the line for
		}
		else
		{
			line.push_back(c);
			//cout << c;
		}
	}

	inFile.close(); //Close the text file

	return strings;
}

std::vector<vector<vector<char>>> Delim2DArray(std::vector<vector<char>> inputArray)
{
	// Vars to build output
	vector<char> newElement;
	vector<vector<char>> newLine;
	vector<vector<vector<char>>> outputArray;
	// Var for reading input
	vector<char> searchArray;

	int i, j;
	for (i = 0; i < inputArray.size(); i++)
	{
		searchArray = inputArray[i];
		newElement.clear(); // Prepare to read the first element of a line
		for (j = 0; j < searchArray.size(); j++)
		{
			if (searchArray[j] == ',') // Delimit one array into elements of 2d array
			{
				newLine.push_back(newElement);
				newElement.clear(); // Prepare for the next element
			}
			else if (searchArray[j] != ' ') // Remove whitespace from the array
			{
				newElement.push_back(searchArray[j]);
				// cout << searchArray[j]; // Debugging characters
			}
		}
		outputArray.push_back(newLine); // Push back the delimited line
		newLine.clear(); // Prepare for the next line
	}

	return outputArray;
}

// Look for the X, Y and Z indices in the input
std::vector<int> findIndices() {
	bool xFound = false, yFound = false, zFound = false;
	vector<int> indices(3); // x, y, z first indices

	int i, j, z;
	for (i = 0; i < delimInput.size(); i++)
	{
		for (j = 0; j < delimInput[i].size(); j++)
		{
			if (xFound == true && yFound == true && zFound == true) { break; }
			if (delimInput[i][j][0] == 'X' && xFound == false)
			{
				xFound = true;
				indices[0] = j;
			}
			if (delimInput[i][j][0] == 'Y' && yFound == false)
			{
				yFound = true;
				indices[1] = j;
			}
			if (delimInput[i][j][0] == 'Z' && zFound == false)
			{
				zFound = true;
				indices[2] = j;
			}
		}
	}

	cout << flush;
	cout << "X Start Index: " << indices[0] << '\n';
	cout << "Y Start Index: " << indices[1] << '\n';
	cout << "Z Start Index: " << indices[2] << '\n';
	cout << '\n';
	return indices;
}

// Read the delimted input array
void readDelimArray() {
	int i, j, z;
	for (i = 0; i < delimInput.size(); i++) // Index through Lines
	{
		for (j = 0; j < delimInput[i].size(); j++) // Index through Elements
		{
			for (z = 0; z < delimInput[i][j].size(); z++) // Index through Characters
			{
				cout << delimInput[i][j][z];
			}
			cout << " ";
		}
		cout << '\n' << '\n';
	}
}

std::vector<vector<vector<char>>> cleanDelimArray() {
	std::vector<vector<char>> cleanLine;
	std::vector<vector<vector<char>>> cleanArray;

	int i, j;
	for (i = 0; i < delimInput.size(); i++) // Index through Lines
	{
		cleanLine.clear(); // Initialize a new line to add elements to
		for (j = 435; j < 571 + 68; j++) // Index through Elements
		{
			cleanLine.push_back(delimInput[i][j]); // Push elements onto the new line
		}
		cleanArray.push_back(cleanLine);
		// cleanLine.clear(); // Prepare for new line
	}
	return cleanArray;
}

char * vectorToCharBuff(vector<char> inArr)
{
	char buff[10];

	int i;
	for (i = 0; i < inArr.size(); i++)
	{
		buff[i] = inArr[i];
	}

	return buff;
}

// Calculate an average over a number of points that desribe the 3d location of a facial landmark
vector<double> landMarkAverage(vector<vector<vector<char>>> inpArray, int startIndex, int endIndex, string whatavg) {
	vector<double> averages;
	double sum = 0;
	double avg;
	char buff[50];

	int i, j, z;
	for (i = 1; i < inpArray.size(); i++) // Index through Lines, line 0 does not contain numerical data
	{
		sum = 0;
		for (j = startIndex; j <= endIndex; j++) // Index through elements relevant to an area
		{
			if (whatavg.compare("x") == 0)
			{
				sum += atof(vectorToCharBuff(inpArray[i][j])); // get the x value
			}
			else if (whatavg.compare("y") == 0)
			{
				sum += atof(vectorToCharBuff(inpArray[i][j + 68])); //get the y value
			}
			else if (whatavg.compare("z") == 0)
			{
				sum += atof(vectorToCharBuff(inpArray[i][j + (68 * 2)])); // get the z value
			}
		}
		avg = sum / (endIndex - startIndex);
		cout << "Frame: " << i << " Coordinate(s): " << whatavg << " Average is: " << avg << '\n';
		averages.push_back(avg);
	}
	return averages;
}

// Main Function
int main(int argc, char **argv) {


	std::string resourceDir = "../resources";
	if (argc >= 2) {
		resourceDir = argv[1];
	}

	Application *application = new Application();

	// Initialize window.
	WindowManager * windowManager = new WindowManager();
	windowManager->init(1280, 720);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// Initialize scene.
	application->init(resourceDir);
	application->initGeom(resourceDir);


	// Read and sort the input text files into an optimized format
	opFaceInput = readOpenFaceOutput();
	delimInput = Delim2DArray(opFaceInput);
	//findIndices(); // Find where 3D landmark output begins in the array and save the indices to an array
	// Results from findIndices(): X indices start at index 435, Y indices start at index 503 and z at 571. Each X, Y and Z has 68 values
	delimInput = cleanDelimArray(); // Clean the array to only contain the necessary informations
	//readDelimArray(); // Output the cleaned up array

	// Calculate Values used to animate the face
	// Relevant X,Y,Z indices for Face
	// Upper Lips: 49 to 53
	// Lower Lip: 54 to 59 and 48
	// Upper Left Eye: 37 and 38
	// Lower Left Eye: 40 and 41
	// Upper Right Eye: 43 and 44
	// Lower Right Eye: 46 and 47
	upperLipAvg = landMarkAverage(delimInput, 49, 53, "y");
	lowerLipAvg = landMarkAverage(delimInput, 54, 59, "y");
	upperLeftEyeAvg = landMarkAverage(delimInput, 37, 38, "y");
	upperRightEyeAvg = landMarkAverage(delimInput, 43, 44, "y");
	lowerLeftEyeAvg = landMarkAverage(delimInput, 40, 41, "y");
	lowerRightEyeAvg = landMarkAverage(delimInput, 46, 47, "y");


	txtarray.push_back('c');
	//bool inpRead = FALSE; //FALSE if not done reading, TRUE if done reading

	// Timing
	t1 = high_resolution_clock::now(); // Get the current time before the loop starts
	charindex = 1; //set initial

	// Loop until the user closes the window.
	while (!glfwWindowShouldClose(windowManager->getHandle())) {
		// Update camera position.
		application->camera->update();
		// Render scene.
		application->render();
		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program. Outside main while loop
	windowManager->shutdown();
	return 0;

}
