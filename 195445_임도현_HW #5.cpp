#define _CRT_SECURE_NO_WARNINGS
#define CUBE_SIZE 1
#define FLOOR_SIZE 50
#define WINDOW_SIZE 760
#define BACKGROUND_COLOR 85
#define PI 3.1415926535
#define ALPHA 0.15

#include <windows.h>
#include <glut.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <math.h>
#include "vector_and_point.h"


GLubyte* PixelBuffer = new GLubyte[WINDOW_SIZE * WINDOW_SIZE * 3];
GLfloat xAngle, yAngle, zAngle;
GLfloat ex = 5, ey = 5, ez = 5;

int mainWindow, window_for_right_eye, window_for_left_eye;

int windowWidth, windowHeight;

struct AlphaAndHit {
    double Alpha;
    bool Hit;
    AlphaAndHit(double Alpha, bool Hit) {
        this->Alpha = Alpha;
        this->Hit = Hit;
    }
};

struct Rect {
    Point_3D startPoint;
    double width;//가로(x 축)
    double length;//세로(z 축)
    double height;//높이(y 축)
    byte color[3];

    Rect(Point_3D startPoint, double width, double length, double height) {
        this->startPoint = startPoint;
        this->width = width;
        this->height = height;
        this->length = length;
    }
};

struct Ball {
    Point_3D center;
    double radius;
    byte color[3];
    Ball(Point_3D center, double radius) {
        this->center = center;
        this->radius = radius;
    }
};

struct Plane {
    Vector_3D Normal;
    Point_3D P0;
    byte color[3];
    Plane(byte r, byte g, byte b) {
        Normal = Vector_3D(0, 1, 0);
        P0 = Point_3D(0, 0, 0);
        color[0] = r;
        color[1] = g;
        color[2] = b;
    }
    Plane(Vector_3D Normal, Point_3D P0) {
        this->Normal = Normal;
        this->P0 = P0;
    }
};

struct HitAndHitPoint {
    bool hit;
    Point_3D hitPoint;

    HitAndHitPoint(bool hit, Point_3D hitPoint) {
        this->hit = hit;
        this->hitPoint = hitPoint;
    }

    HitAndHitPoint(bool hit) {
        this->hit = hit;
    }
};

// 구현해야 할 것
// 1. 눈에서 화면으로 광선을 쏘는 함수
//  + 시좌표계를 변환해야 함. 성공
// 2. 광원
// 3. 반사
//  + 일단 충돌함수부터 구현할 것. 레이 트레이싱의 가장 기초인 원 부터 해보자.
// 4. 쉐이딩

void initPixel(GLubyte* pixels, int width, int height) {
    for (int i = 0; i < width * height * 3; i++) {
        pixels[i] = BACKGROUND_COLOR;
    }
}

void makePixel(int x, int y, int r, int g, int b, GLubyte* pixels, int width, int height) {
    if (0 <= x && x < width && 0 <= y && y < height) {
        int position = (x * width + y) * 3;
        pixels[position] = r;
        pixels[position + 1] = g;
        pixels[position + 2] = b;
    }
}

void change(double* x, double* y) {
    double temp = *x;
    *x = *y;
    *y = temp;
}

HitAndHitPoint Plane_hit(Vector_3D ray, Plane plane, Point_3D& eye) {
    if (ray * plane.Normal == 0) {
        return HitAndHitPoint(false);
    }
    else {
        double t = -1 * (((eye - plane.P0) * plane.Normal) / (ray * plane.Normal));
        //cout << t << " t " << endl;
        if (t > 1000 || t < 0) return HitAndHitPoint(false);
        else {
            Point_3D hit_point = eye + ray * t;
            return HitAndHitPoint(true, hit_point);
        }
    }
}

HitAndHitPoint Rectangle_hit(Vector_3D ray, Rect Rectangle, Point_3D& eye) {
    Vector_3D x_vec(1, 0, 0);
    Vector_3D y_vec(0, 1, 0);
    Vector_3D z_vec(0, 0, 1);

    Point_3D x_point(Rectangle.startPoint.get_x() + Rectangle.width, 0, 0);
    Point_3D y_point(0, Rectangle.startPoint.get_y() + Rectangle.height, 0);
    Point_3D z_point(0, 0, Rectangle.startPoint.get_z() + Rectangle.length);

    Plane yz_plane(x_vec, x_point);
    Plane xz_plane(y_vec, y_point);
    Plane xy_plane(z_vec, z_point);

    HitAndHitPoint yz_rect = Plane_hit(ray, yz_plane, eye);
    HitAndHitPoint xz_rect = Plane_hit(ray, xz_plane, eye);
    HitAndHitPoint xy_rect = Plane_hit(ray, xy_plane, eye);

    double x_val;
    double y_val;
    double z_val;

    double x_min = Rectangle.startPoint.get_x();
    double y_min = Rectangle.startPoint.get_y();
    double z_min = Rectangle.startPoint.get_z();

    double x_max = Rectangle.startPoint.get_x() + Rectangle.width;
    double y_max = Rectangle.startPoint.get_y() + Rectangle.height;
    double z_max = Rectangle.startPoint.get_z() + Rectangle.length;

    if (yz_rect.hit) {
        y_val = yz_rect.hitPoint.get_y();
        z_val = yz_rect.hitPoint.get_z();
        if ((y_min < y_val && y_val < y_max) && (z_min < z_val && z_val < z_max)) {
            return yz_rect;
        }
    }
    if (xz_rect.hit) {
        x_val = xz_rect.hitPoint.get_x();
        z_val = xz_rect.hitPoint.get_z();
        if ((x_min < x_val && x_val < x_max) && (z_min < z_val && z_val < z_max)) {
            return xz_rect;
        }
    }
    if (xy_rect.hit) {
        y_val = xy_rect.hitPoint.get_y();
        x_val = xy_rect.hitPoint.get_x();
        if ((y_min < y_val && y_val < y_max) && (x_min < x_val && x_val < x_max)) {
            return xy_rect;
        }
        else {
            return HitAndHitPoint(false);
        }
    }
    if (!(yz_rect.hit || xz_rect.hit || xy_rect.hit)) {
        return HitAndHitPoint(false);
    }

}


HitAndHitPoint Ball_hit(Vector_3D ray, Ball ball, Point_3D& eye) {
    Vector_3D CO = eye - ball.center;
    double a = ray * ray;
    double b = ray * CO;
    double c = CO * CO - (ball.radius * ball.radius);

    double d = b * b - a * c;
    if (d > 0) {
        double t = (-1 * b - sqrt(d)) / a;
        Point_3D hit_point = eye + ray * t;
        return HitAndHitPoint(true, hit_point);
    }
    else {
        return HitAndHitPoint(false);
    }
}

double angle_for_shading(Point_3D point, Ball ball, Point_3D light) {
    Vector_3D PL = light - point;
    Vector_3D Normal = point - ball.center;
    double cos = (PL * Normal) / (PL.absolute_val() * Normal.absolute_val());
    return cos;
}
double angle_for_shading_rect(Point_3D point, Rect rect, Point_3D light) {
    Vector_3D PL = light - point;
    Vector_3D Normal;
    if (rect.startPoint.get_y() + rect.height == point.get_y()) {
        Normal = Vector_3D(0, 1, 0);
    }
    else if (rect.startPoint.get_z() + rect.length == point.get_z()) {
        Normal = Vector_3D(0, 0, 1);
    }
    else {
        Normal = Vector_3D(1, 0, 0);
    }
    double cos = (PL * Normal) / (PL.absolute_val() * Normal.absolute_val());
    return cos;
}

AlphaAndHit shadow(Point_3D point, Ball ball, Point_3D light) {
    Vector_3D light_to_floor = point - light;

    HitAndHitPoint floor_to_ball = Ball_hit(light_to_floor, ball, light);
    if (floor_to_ball.hit) {
        return AlphaAndHit(0.1, true);
    }
    else {
        return AlphaAndHit(1, false);
    }
}

AlphaAndHit shadow_rect(Point_3D point, Rect rect, Point_3D light) {
    Vector_3D light_to_floor = point - light;

    HitAndHitPoint floor_to_ball = Rectangle_hit(light_to_floor, rect, light);
    if (floor_to_ball.hit) {
        return AlphaAndHit(0.1, true);
    }
    else {
        return AlphaAndHit(1, false);
    }
}

Vector_3D convert_eye_to_world(
    vector<vector<double>> convert_matrix,
    Vector_3D eye_cordinate_sys_vec,
    Point_3D eye_point
) {
    vector<double> eye_cordinate_vec = eye_cordinate_sys_vec.return_list();
    eye_cordinate_vec.push_back(0);// 벡터임을 표시

    vector<double> world_cordinate_vec;

    for (int i = 0; i < 4; i++) {
        double sum = 0;
        for (int j = 0; j < 4; j++) {
            sum += convert_matrix[i][j] * eye_cordinate_vec[j];
        }
        world_cordinate_vec.push_back(sum);
    }
    Vector_3D return_val(world_cordinate_vec[0], world_cordinate_vec[1], world_cordinate_vec[2]);
    return_val.to_unit_vector();
    return return_val;
}

vector<vector<Vector_3D>> eye_to_screen(int ex, int ey, int ez, int height, int width) {
    Point_3D eye_world_point(ex, ey, ez);
    // 무조건 머리가 y축을 향한다고 가정.
    Vector_3D up_world_vector(0, 1, 0);
    // 우리가 바라보고자 하는 지점
    Point_3D direction_world_point(0, 0, 0);

    Vector_3D direction_vector = direction_world_point - eye_world_point;

    Vector_3D u_vector = direction_vector.cross_product(up_world_vector); // 시좌표게의 x축을 world cordinate system으로 변환
    Vector_3D v_vector = u_vector.cross_product(direction_vector); // 시좌표게의 y축을 world cordinate system으로 변환
    Vector_3D w_vector = direction_vector * -1; // 시좌표계의 z축을 world cordinate system으로 변환\


    u_vector.to_unit_vector();
    v_vector.to_unit_vector();
    w_vector.to_unit_vector();


    // 시좌표계에서 눈은 항상 원점
    Point_3D eye_point(0, 0, 0);

    vector<vector<Vector_3D>>ray(height); // 각 픽셀마다 나가는 벡터를 저장하기 위한 리스트

    vector<vector<double>> convert_matrix(4);

    vector<double> u_list = u_vector.return_list();
    vector<double> v_list = v_vector.return_list();
    vector<double> w_list = w_vector.return_list();
    vector<double> eye = eye_world_point.return_list();

    for (int i = 0; i < 4; i++) {
        if (i < 3) {
            convert_matrix[i].push_back(u_list[i]);
            convert_matrix[i].push_back(v_list[i]);
            convert_matrix[i].push_back(w_list[i]);
            convert_matrix[i].push_back(eye[i]);
        }
        else {
            for (int j = 0; j < 4; j++) {
                convert_matrix[i].push_back(0);
            }
        }
    }
    convert_matrix[3][3] = 1;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            Point_3D eye_to_screen(j - width / 2 + 0.5, i - height / 2 + 0.5, -width / 2); // 시야각이 90도라고 가정한 상태
            Vector_3D eye_cordinate_lay = eye_to_screen - eye_point; // 시좌표계에서 본 눈에서 스크린으로 쏜 광선 벡터
            Vector_3D world_cordinate_lay = convert_eye_to_world(convert_matrix, eye_cordinate_lay, eye_world_point);
            ray[i].push_back(world_cordinate_lay);
        }
    }
    return ray;
}

void ray_casting(vector<vector<Vector_3D>> ray, int width, int height, GLubyte* pixels, int ex, int ey, int ez) {
    Ball ball(Point_3D(0, 80, 0), 80);

    ball.color[0] = 255;
    ball.color[1] = 51;
    ball.color[2] = 204;

    Plane plane(10, 10, 10);

    Point_3D for_rectangle(-20, 0, 100);
    Rect rectangle(for_rectangle, 40, 80, 100);
    rectangle.color[0] = 106;
    rectangle.color[1] = 133;
    rectangle.color[2] = 24;

    Point_3D light(100, 1300, 1300);
    Point_3D eye(ex, ey, ez);
    double threshold = 0.94;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            HitAndHitPoint plane_hit = Plane_hit(ray[i][j], plane, eye);
            HitAndHitPoint ball_hit = Ball_hit(ray[i][j], ball, eye);
            HitAndHitPoint rectangle_hit = Rectangle_hit(ray[i][j], rectangle, eye);

            AlphaAndHit ball_hit_shadow = shadow(plane_hit.hitPoint, ball, light);
            AlphaAndHit rectangle_hit_shadow = shadow_rect(plane_hit.hitPoint, rectangle, light);

            if (plane_hit.hit) {
                long x = plane_hit.hitPoint.get_x();
                long z = plane_hit.hitPoint.get_z();
                if (ball_hit_shadow.Hit) {
                    double shadow_val = ball_hit_shadow.Alpha;

                    if (((x / 20) % 2 + (z / 20) % 2) % 2 == 0) {
                        makePixel(i, j, (plane.color[0]) * shadow_val, (plane.color[1]) * shadow_val, (plane.color[2]) * shadow_val, pixels, width, height);
                    }
                    else {
                        makePixel(i, j, (255 - plane.color[0]) * shadow_val, (255 - plane.color[1]) * shadow_val, (255 - plane.color[2]) * shadow_val, pixels, width, height);
                    }
                }
                else if (rectangle_hit_shadow.Hit) {
                    double shadow_val = rectangle_hit_shadow.Alpha;
                    if (((x / 20) % 2 + (z / 20) % 2) % 2 == 0) {
                        makePixel(i, j, (plane.color[0]) * shadow_val, (plane.color[1]) * shadow_val, (plane.color[2]) * shadow_val, pixels, width, height);
                    }
                    else {
                        makePixel(i, j, (255 - plane.color[0]) * shadow_val, (255 - plane.color[1]) * shadow_val, (255 - plane.color[2]) * shadow_val, pixels, width, height);
                    }
                }
                else {
                    double shadow_val = 1;
                    if (((x / 20) % 2 + (z / 20) % 2) % 2 == 0) {
                        makePixel(i, j, (plane.color[0]) * shadow_val, (plane.color[1]) * shadow_val, (plane.color[2]) * shadow_val, pixels, width, height);
                    }
                    else {
                        makePixel(i, j, (255 - plane.color[0]) * shadow_val, (255 - plane.color[1]) * shadow_val, (255 - plane.color[2]) * shadow_val, pixels, width, height);
                    }
                }

            }
            if (ball_hit.hit) {
                Ball temp = ball;
                double cos = angle_for_shading(ball_hit.hitPoint, ball, light);
                double x = pow(cos, 5);
                if (x > threshold) {
                    double alpha_r = ((1 - x) / (1 - threshold)) * temp.color[0] + ((x - threshold) / (1 - threshold)) * 255;
                    double alpha_g = ((1 - x) / (1 - threshold)) * temp.color[1] + ((x - threshold) / (1 - threshold)) * 255;
                    double alpha_b = ((1 - x) / (1 - threshold)) * temp.color[2] + ((x - threshold) / (1 - threshold)) * 255;
                    temp.color[0] = (byte)alpha_r;
                    temp.color[1] = (byte)alpha_g;
                    temp.color[2] = (byte)alpha_b;
                }
                else {
                    double alpha = (1 - cos) * ALPHA / 2 + (1 + cos) / 2;
                    temp.color[0] = (byte)temp.color[0] * alpha;
                    temp.color[1] = (byte)temp.color[1] * alpha;
                    temp.color[2] = (byte)temp.color[2] * alpha;
                }
                makePixel(i, j, temp.color[0], temp.color[1], temp.color[2], pixels, width, height);
            }
            if (rectangle_hit.hit) {
                Rect temp = rectangle;
                double cos = angle_for_shading_rect(rectangle_hit.hitPoint, rectangle, light);
                double alpha = (1 - cos) * ALPHA / 2 + (1 + cos) / 2;
                temp.color[0] = (byte)temp.color[0] * alpha;
                temp.color[1] = (byte)temp.color[1] * alpha;
                temp.color[2] = (byte)temp.color[2] * alpha;
                makePixel(i, j, temp.color[0], temp.color[1], temp.color[2], pixels, width, height);
            }

        }
    }

}


void DoInit() {
    glClearColor(0.3, 0.3, 0.3, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    initPixel(PixelBuffer, WINDOW_SIZE, WINDOW_SIZE);
}

void DoDisplayForMainTest()
{
    //int ex = 200;
    //int ey = 100;
    //int ez = 300;
    DoInit();
    //makePixel(380, 380, 255, 255, 255, PixelBuffer, WINDOW_SIZE, WINDOW_SIZE);
    vector<vector<Vector_3D >>ray = eye_to_screen(ex, ey, ez, WINDOW_SIZE, WINDOW_SIZE);
    //ray_casting(ray, WINDOW_SIZE, WINDOW_SIZE, PixelBuffer, ex, ey, ez);
    //glDrawPixels(WINDOW_SIZE, WINDOW_SIZE, GL_RGB, GL_UNSIGNED_BYTE, PixelBuffer);
    glutSwapBuffers();
}

void DoDisplayForRightEye()
{
    int ex = 210;
    int ey = 100;
    int ez = 290;
    DoInit();
    //makePixel(380, 380, 255, 255, 255, PixelBuffer, WINDOW_SIZE, WINDOW_SIZE);
    vector<vector<Vector_3D >>ray = eye_to_screen(ex, ey, ez, WINDOW_SIZE, WINDOW_SIZE);
    ray_casting(ray, WINDOW_SIZE, WINDOW_SIZE, PixelBuffer, ex, ey, ez);
    glDrawPixels(WINDOW_SIZE, WINDOW_SIZE, GL_RGB, GL_UNSIGNED_BYTE, PixelBuffer);
    glutSwapBuffers();
}

void DoDisplayForLeftEye()
{
    int ex = 190;
    int ey = 100;
    int ez = 310;
    DoInit();
    //makePixel(380, 380, 255, 255, 255, PixelBuffer, WINDOW_SIZE, WINDOW_SIZE);
    vector<vector<Vector_3D >>ray = eye_to_screen(ex, ey, ez, WINDOW_SIZE, WINDOW_SIZE);
    ray_casting(ray, WINDOW_SIZE, WINDOW_SIZE, PixelBuffer, ex, ey, ez);
    glDrawPixels(WINDOW_SIZE, WINDOW_SIZE, GL_RGB, GL_UNSIGNED_BYTE, PixelBuffer);
    glutSwapBuffers();
}


void renderSceneForMain() {
    glutSetWindow(mainWindow);
    DoDisplayForMainTest();
}

void renderSceneForRightEye() {
    glutSetWindow(window_for_right_eye);
    DoDisplayForRightEye();
}

void renderSceneForLeftEye() {
    glutSetWindow(window_for_left_eye);
    DoDisplayForLeftEye();
}

int main(int argc, char** argv)
{
    int border = 40;
    windowWidth = (WINDOW_SIZE + border) * 2;
    windowHeight = WINDOW_SIZE;


    glutInit(&argc, argv);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

    mainWindow = glutCreateWindow("OpenGL");
    glutDisplayFunc(renderSceneForMain);

    window_for_right_eye = glutCreateSubWindow(mainWindow, border / 2, border / 2, windowWidth / 2 - border, windowHeight - border);
    glutDisplayFunc(renderSceneForRightEye);

    window_for_left_eye = glutCreateSubWindow(mainWindow, windowWidth / 2 + border / 2, border / 2, windowWidth / 2 - border, windowHeight - border);
    glutDisplayFunc(renderSceneForLeftEye);


    glutMainLoop();
    return 0;
}

