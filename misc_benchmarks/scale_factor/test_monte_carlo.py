import numpy as np


def monte_carlo(radius, rand_samples, scale_factor, sqrt7):
    scale = rand_samples * scale_factor
    r = radius * scale

    def y1_upper(x): return r + np.sqrt(r ** 2 - (x - r) ** 2)

    def y1_lower(x): return r - np.sqrt(r ** 2 - (x - r) ** 2)

    def y2(x): return np.sqrt(4 * r**2 - (x - 2*r)**2)
    rand_x = np.random.randint(0, 2*r+1, rand_samples)
    rand_y = np.random.randint(0, 2*r+1, rand_samples)
    inside = 0
    for i in range(rand_samples):
        x_dot = rand_x[i]
        y_dot = rand_y[i]
        if 0 <= x_dot < (3-sqrt7)/4*r:
            if y1_lower(x_dot) <= y_dot < y1_upper(x_dot):
                inside += 1
        if (3-sqrt7)/4*r <= x_dot < (3+sqrt7)/4*r:
            if y2(x_dot) <= y_dot < y1_upper(x_dot):
                inside += 1
    return inside/rand_samples * 4 * radius**2


def do_test(every, rand_samples):
    sqrt7 = np.sqrt(7)
    # 重要常量
    r = 5
    # 随机点数
    results = []
    realsize = (np.pi-np.arccos(-np.sqrt(2)/4)-4*np.arccos(5*np.sqrt(2)/8)+2*np.sin(2 *
                                                                                    np.arccos(5*np.sqrt(2)/8))-0.5*np.sin(2*np.pi-2*np.arccos(-np.sqrt(2)/4)))*r**2
    for sf in np.arange(0.1, 10.1, 0.1):
        result = 0
        for j in range(every):
            size = monte_carlo(r, rand_samples, sf, sqrt7)
            error = abs(size-realsize)
            result += error
        results.append(result/every)
    return results
