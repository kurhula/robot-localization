#include <iostream>
#include <SDL.h>
#include <vector>
#include "Eigen/Dense"

#include "Robot.h"
#include "Landmark.h"
#include "KalmanFilter.h"


std::vector<Landmark> createLandmarks()
{
    std::vector<Landmark> lmks;

    SDL_Color color_red = {.r = 255, .g = 0, .b = 0, .a = 255 };
    SDL_Color color_green = {.r = 0, .g = 255, .b = 0, .a = 255 };
    SDL_Color color_blue = {.r = 0, .g = 0, .b = 255, .a = 255 };
    SDL_Color color_purple = {.r = 255, .g = 0, .b = 255, .a = 255 };

    lmks.push_back( Landmark(100.,100.,color_red));
    lmks.push_back( Landmark(300.,300.,color_green));
//    lmks.push_back( Landmark(124.,478.,color_blue));
//    lmks.push_back( Landmark(214.,312.,color_purple));

    return lmks;
}


int main() {

    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }


    SDL_Window *win = SDL_CreateWindow("Robot Program", XSTART, YSTART, WWIDTH, WHEIGHT, SDL_WINDOW_SHOWN);
    if (win == nullptr){
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == nullptr){
        SDL_DestroyWindow(win);
        std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }


    std::vector<Landmark> landmarks = createLandmarks();

    SDL_Color orange {.r=255, .g=165, .b=0, .a=255};
    SDL_Color red {.r=255, .g=0, .b=0, .a=255};
    SDL_Color gray {.r=128, .g=128, .b=128, .a=255};


    Robot robby(200, 200, 0.0, 20, red);
    Robot robby_estimate(200, 200, 0.0, 20, gray);

    // Kalman filter stuff

    int n = 3; // number of state variables (x,y,phi)
    int m = 3; // number of measurements (landmark x, y, index)


    // Best guess of initial states
    Eigen::VectorXf x0(n); //[x, y, phi]

    // control vector:
    Eigen::VectorXf control(2); //  [v, omega]


    x0 << 200., 200., 0.0;

    Eigen::MatrixXf A(n, n); // System dynamics matrix
    Eigen::MatrixXf C(m, n); // Output matrix
    Eigen::MatrixXf Q(n, n); // Process noise covariance
    Eigen::MatrixXf R(m, m); // Measurement noise covariance
    Eigen::MatrixXf covariance(n, n); // Estimate error covariance


    // Reasonable covariance matrices
    covariance <<    5., .0, .0,
                     .0, 5., .0,
                    .0, .0,  5.;


    R <<    2.0, 0., 0.,
            0., 2.0, 0.,
            0., 0.,  2.0;

    Q <<    0.1, 0.1, 0.1,
            0.1, 0.1, 0.1,
            0.1, 0.1, 0.1;


    KalmanFilter kf(DT, A, C, Q, R, covariance);


    float t0 = 0.0;
    kf.init(t0, x0);

    // rendering loop
    int i = 0;
    while (true) {

        //First clear the renderer
        SDL_RenderClear(ren);

        //Draw the texture
        SDL_SetRenderDrawColor(ren, 200, 200, 255, 255);

        SDL_RenderClear(ren); // fill the scene with white

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);

        // update renderer
        SDL_RenderPresent(ren);

        // move robot
        SDL_PumpEvents();
        const Uint8 *key_pressed = SDL_GetKeyboardState(NULL);
        robby.move(key_pressed, control);

        // measure landmark positions
        auto observed_landmarks = robby.measureLandmarks(landmarks);

        // get robot state
        Eigen::VectorXf state = robby.get_state();

        // Localize via Landmarks
        kf.localization_landmarks(observed_landmarks, landmarks, control);

        // get estimated state (x,y,phi) as estimated by the EKF
        auto x_hat = kf.get_state();

        printf("True x,y,phi: %f, %f, %f\n", state(0), state(1),state(2));
        printf("Estimated x,y,phi: %f, %f, %f\n", x_hat(0), x_hat(1), x_hat(2));

        robby_estimate.setPose(x_hat(0), x_hat(1), x_hat(2));
        robby_estimate.render(ren);

        // render robot
        robby.render(ren);

        // render true landmarks
        for (auto lm = landmarks.begin(); lm != landmarks.end(); ++lm)
        {
            SDL_SetRenderDrawColor(ren, lm->id.r, lm->id.g, lm->id.b, lm->id.a);
            lm->render(ren);
        }
        // render observed landmarks
        for (auto lm = observed_landmarks.begin(); lm != observed_landmarks.end(); ++lm)
        {
            SDL_SetRenderDrawColor(ren, lm->id.r, lm->id.g, lm->id.b, lm->id.a);
            lm->render(ren);
        }


        SDL_SetRenderDrawColor(ren, gray.r, gray.g, gray.b, gray.a);
        kf.renderSamples(ren);


        SDL_RenderPresent(ren);


        //Take a quick break after all that hard work
        SDL_Delay(30);


        if (key_pressed[SDL_SCANCODE_RETURN])
        {
            printf("Exiting game!\n");
            break;
        }

        i+=1;
    }


    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();



    return 0;
}