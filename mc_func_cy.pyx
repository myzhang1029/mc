# cython: cdivision=True, wraparound=False, boundscheck=False
#
# Standard Functional Monte Carlo Cython code
# Copyright 2019-2022 Zhang Maiyun <me@maiyun.me>
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
from libc.math cimport sqrt, pi, acos, sin
from random import SystemRandom

cdef double y1_upper(double x, int r) nogil:
    return r + sqrt(-x**2 + 2*x*r)

cdef double y1_lower(double x, int r) nogil:
    return r - sqrt(-x**2 + 2*x*r)

cdef double y2(double x, int r) nogil:
    return sqrt(-x**2 + 4*x*r)

cdef double monte_carlo(int radius, int rand_samples, double scale_factor, double sqrt7):
    cdef int scale = int(rand_samples * scale_factor)
    cdef int r = radius * scale
    cdef int x_dot, y_dot
    cdef int inside = 0

    for i in range(rand_samples):
        x_dot = SystemRandom().randint(0, 2*r)
        y_dot = SystemRandom().randint(0, 2*r)
        if 0 <= x_dot < (3 - sqrt7)/4*r:
            if y1_lower(x_dot, r) <= y_dot < y1_upper(x_dot, r):
                inside += 1
        if (3 - sqrt7)/4*r <= x_dot < (3 + sqrt7)/4*r:
            if y2(x_dot, r) <= y_dot < y1_upper(x_dot, r):
                inside += 1
    return inside/float(rand_samples) * 4 * radius**2

def calculate(int r, int rand_samples):
    cdef double sqrt7 = sqrt(7)
    return monte_carlo(r, rand_samples, 15, sqrt7)

def do_test(int every, int rand_samples):
    cdef double result, size, error, sqrt7 = sqrt(7)
    cdef int sf, j, r = 5
    results = []
    cdef double realsize = (pi-acos(-sqrt(2)/4)-4*acos(5*sqrt(2)/8)+2*sin(2 * acos(5*sqrt(2)/8))-0.5*sin(2*pi-2*acos(-sqrt(2)/4)))*r**2
    for sf in range(1, 101):
        result = 0
        for j in range(every):
            size = monte_carlo(r, rand_samples, sf * 0.1, sqrt7)
            error = abs(size-realsize)
            result += error
        results.append(result/every)
    return results
