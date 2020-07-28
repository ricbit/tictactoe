import matplotlib.pyplot as plot

pn = [x.split() for x in open("pnevolution.txt", "rt").readlines()]
plot.plot([int(x[0]) for x in pn])
plot.plot([int(x[1]) for x in pn])
plot.show()
