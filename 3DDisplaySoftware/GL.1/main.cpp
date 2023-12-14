#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/glext.h>
#pragma comment(lib, "glew32.lib") 

#include "SerialPort.h";
#include "USB.h"
#include <thread>

using namespace std;

USB *usb;

const float screen_width_cm = 34.5;
const float screen_height_cm = 19.4;
const float out_screen_coverage = 0.8;
const float real_far_plate_cm = -100.0;

const float sensor_pos_x = -17, sensor_pos_y = -10, sensor_pos_z = 24;
//const float sensor_pos_x = 0, sensor_pos_y = 0, sensor_pos_z = 0;

static float refresh_time = (1000.0 / 60.0) * 4.1 ;
//static float refresh_time = (1000.0 / 59.0);

//static float eye_l_x = -3.5, eye_l_y = 10, eye_l_z = 30;
//static float eye_r_x = 3.5, eye_r_y = 10, eye_r_z = 30;

static float eye_l_x = -33.5, eye_l_y = 15.0, eye_l_z = 36.0;
static float eye_r_x = -29.5, eye_r_y = 15.0, eye_r_z = 40.0;
static float eye_x = 0.0, eye_y = 0.0, eye_z = 40.0; // all in cm
static bool parity = 0;

static float yRotationAngle = 0.0;
static float sphereAngle = 0.0;

bool CMD = false;
unsigned char data;

// Drawing routine.
void drawScene(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(1.0, 1.0, 1.0);
	glLoadIdentity();

	float real_near = eye_z * out_screen_coverage;

	glFrustum(
		-screen_width_cm / 2.0 + real_near*(eye_x + screen_width_cm / 2.0) / eye_z - eye_x,
		screen_width_cm / 2.0 + real_near*(eye_x - screen_width_cm / 2.0) / eye_z - eye_x,
		-screen_height_cm / 2.0 + real_near*(eye_y + screen_height_cm / 2.0) / eye_z - eye_y,
		screen_height_cm / 2.0 + real_near*(eye_y - screen_height_cm / 2.0) / eye_z - eye_y,
		-(real_near - eye_z), -(real_far_plate_cm - eye_z)
		);

	glTranslatef(-eye_x, -eye_y, -eye_z);

	if (parity){
		glBegin(GL_POLYGON);
		glVertex3f(-17.25, -9.7, 0.0);
		glVertex3f(-17.25, -8.7, 0.0);
		glVertex3f(-16.25, -8.7, 0.0);
		glVertex3f(-16.25, -9.7, 0.0);
		glEnd();
	}

	glPushMatrix();
	glRotatef(yRotationAngle, 0.0f, 1.0f, 0.0f);
	//glutWireSphere(2.5, 30, 30);
	glutWireTeapot(5.0);
	glPopMatrix();

	glPushMatrix();
	glColor3f(0.2, 1.0, 1.0);
	glRotatef(sphereAngle, 0.0f, 1.0f, 0.0f);
	glTranslatef(10, 0, 0);
	//glutWireSphere(2.5, 30, 30);
	glutWireSphere(2.0, 20, 20);
	glPopMatrix();

	glFlush();
}

void RefreshTimer(int t){
	glutTimerFunc(t, RefreshTimer, t);


	usb->sendData(0); // close both eyes
	parity = !parity;

	if (parity){
		eye_x = eye_l_x;
		eye_y = eye_l_y;
		eye_z = eye_l_z;

		data = 2;
	}
	else{
		eye_x = eye_r_x;
		eye_y = eye_r_y;
		eye_z = eye_r_z;

		data = 1;
	}
	sphereAngle += 2.0;
	yRotationAngle -= 1.5;
	glutPostRedisplay();

	CMD = true;
}

// Initialization routine.
void setup(void)
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
}

// OpenGL window reshape routine.
void resize(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
}

void keyInput(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:
		exit(0);
		break;
	case 'a':
		refresh_time += 1.0;
		glutPostRedisplay();
		break;
	case 'z':
		refresh_time -= 1.0;
		glutPostRedisplay();
		break;
	default:
		break;
	}
}

void specialKeyInput(int key, int x, int y)
{
	if (key == GLUT_KEY_UP) eye_y += 2.5;
	if (key == GLUT_KEY_DOWN) eye_y -= 2.5;
	if (key == GLUT_KEY_RIGHT) yRotationAngle += 2.5;
	if (key == GLUT_KEY_LEFT) yRotationAngle -= 2.5;
	glutPostRedisplay();
}

struct Position{
	double x, y, z;
};

Position Clac_Pos(double l0, double l1, double l2){
	Position pos;
	double divider = pow(l2*l2 + l1*l1 + l0*l0, 3.0 / 4.0);
	pos.z = l0 / divider;
	pos.y = l1 / divider;
	pos.x = l2 / divider;
	return pos;
}

double Smooth(double pre_val, double new_val){
	double dif = abs(new_val - pre_val);
	double change = pow(dif, 0.5);
	if (new_val < pre_val){
		return pre_val - change;
	}
	return pre_val + change;
}

void EyeTracker(){
	SerialPort serial_port;

	serial_port.Write('s');
	Sleep(1000);

	int sensor_data_off[3], sensor_data_on[3];
	int lumination_equivalent_voltage[3];
	const int unit_direct_result = 1400; // 
	const double lumination_standardizer = 1.0 / unit_direct_result;
	double standard_lumination[3];
	Position pos;
	const double unit_in_cm = 20.0;

	while (1){
		serial_port.Write('c');
		for (int led = 0; led < 2; led++){
			for (int sensor = 0; sensor < 3; sensor++){
				sensor_data_off[sensor] = serial_port.ReadWord();
				sensor_data_on[sensor] = serial_port.ReadWord();
				lumination_equivalent_voltage[sensor] = sensor_data_on[sensor] - sensor_data_off[sensor];
				standard_lumination[sensor] = lumination_equivalent_voltage[sensor] * lumination_standardizer;
			}
			pos = Clac_Pos(standard_lumination[0], standard_lumination[1], standard_lumination[2]);
			pos.x *= unit_in_cm;
			pos.y *= unit_in_cm;
			pos.z *= unit_in_cm;
			if (!led){
				eye_l_x = Smooth(eye_l_x, pos.x + sensor_pos_x);
				eye_l_y = Smooth(eye_l_y, pos.y + sensor_pos_y);
				eye_l_z = Smooth(eye_l_z, pos.z + sensor_pos_z);
			}
			else{
				eye_r_x = Smooth(eye_r_x, pos.x + sensor_pos_x);
				eye_r_y = Smooth(eye_r_y, pos.y + sensor_pos_y);
				eye_r_z = Smooth(eye_r_z, pos.z + sensor_pos_z);
			}
		}
	}
}

void DelayedSync(){
	while (true){
		if (CMD){
			CMD = false;
			Sleep(60);
			usb->sendData(data);
		}
	}
}

// Main routine.
int main(int argc, char **argv)
{
	thread SerialPortHandler(EyeTracker);

	thread delayedSync(DelayedSync);

	usb = new USB();

	glutInit(&argc, argv);

	glutInitContextVersion(4, 3);
	glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);

	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(500, 500);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("3DGlasses.cpp");
	glutFullScreen();
	glutDisplayFunc(drawScene);
	glutReshapeFunc(resize);
	glutKeyboardFunc(keyInput);
	glutSpecialFunc(specialKeyInput);

	glewExperimental = GL_TRUE;
	glewInit();

	setup();

	glutTimerFunc(refresh_time, RefreshTimer, refresh_time);

	glutMainLoop();
}

