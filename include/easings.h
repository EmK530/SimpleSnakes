#ifndef EASINGS_H
#define EASINGS_H

#include <math.h>

/* ---------- QUAD ---------- */
static inline double easeInQuad(double t) {
    return t * t;
}

static inline double easeOutQuad(double t) {
    return 1 - (1 - t) * (1 - t);
}

static inline double easeInOutQuad(double t) {
    return t < 0.5 ? 2 * t * t : 1 - pow(-2 * t + 2, 2) / 2;
}

/* ---------- CUBIC ---------- */
static inline double easeInCubic(double t) {
    return t * t * t;
}

static inline double easeOutCubic(double t) {
    return 1 - pow(1 - t, 3);
}

static inline double easeInOutCubic(double t) {
    return t < 0.5 ? 4 * t * t * t : 1 - pow(-2 * t + 2, 3) / 2;
}

/* ---------- QUART ---------- */
static inline double easeInQuart(double t) {
    return t * t * t * t;
}

static inline double easeOutQuart(double t) {
    return 1 - pow(1 - t, 4);
}

static inline double easeInOutQuart(double t) {
    return t < 0.5 ? 8 * t * t * t * t : 1 - pow(-2 * t + 2, 4) / 2;
}

/* ---------- QUINT ---------- */
static inline double easeInQuint(double t) {
    return t * t * t * t * t;
}

static inline double easeOutQuint(double t) {
    return 1 - pow(1 - t, 5);
}

static inline double easeInOutQuint(double t) {
    return t < 0.5 ? 16 * t * t * t * t * t : 1 - pow(-2 * t + 2, 5) / 2;
}

/* ---------- SINE ---------- */
static inline double easeInSine(double t) {
    return 1 - cos((t * M_PI) / 2);
}

static inline double easeOutSine(double t) {
    return sin((t * M_PI) / 2);
}

static inline double easeInOutSine(double t) {
    return -(cos(M_PI * t) - 1) / 2;
}

/* ---------- EXPO ---------- */
static inline double easeInExpo(double t) {
    return t == 0 ? 0 : pow(2, 10 * (t - 1));
}

static inline double easeOutExpo(double t) {
    return t == 1 ? 1 : 1 - pow(2, -10 * t);
}

static inline double easeInOutExpo(double t) {
    if (t == 0) return 0;
    if (t == 1) return 1;
    return t < 0.5 ?
        pow(2, 20 * t - 10) / 2 :
        (2 - pow(2, -20 * t + 10)) / 2;
}

/* ---------- CIRC ---------- */
static inline double easeInCirc(double t) {
    return 1 - sqrt(1 - t * t);
}

static inline double easeOutCirc(double t) {
    double u = t - 1;
    return sqrt(1 - u * u);
}

static inline double easeInOutCirc(double t) {
    return t < 0.5
        ? (1 - sqrt(1 - (4 * t * t))) / 2
        : (sqrt(1 - pow(-2 * t + 2, 2)) + 1) / 2;
}

#endif // EASINGS_H