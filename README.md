# mc

My HPC attempts - Monte Carlo method to calculate areas

Question that is calculated:  
A square has an inscribed circle of radius $r$. Another circle is made with
the lower right vertex of the square as the center and $2r$ as the radius.
The two circles intersect at two points. Hence, what is the smaller area
between the two circles?

With trigonometry, the exact value is
$$
r^2\left[\pi-\arccos\frac{-\sqrt{2}}{4}-4\arccos\frac{5\sqrt{2}}{8}+2\sin\left(2\arccos\frac{5\sqrt{2}}{8}\right)-\frac{1}{2}\sin\left(2\pi-2\arccos\frac{-\sqrt{2}}{4}\right)\right]
$$
which is approximately $14.638125953034784$.

However, here I try to solve with Monte Carlo methods, and push the limits
of computation.
