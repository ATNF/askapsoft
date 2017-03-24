#!/usr/bin/env sh
. ./init_package_env.sh

python << EOF
import matplotlib
matplotlib.use('Agg')
print matplotlib.get_backend()
import matplotlib.pyplot as plt
import numpy as np
plt.plot(range(10), range(10))
plt.plot(np.arange(10), np.arange(10)/2.)
plt.title("Simple Plot")
plt.show()
plt.savefig('simple.png')
EOF
