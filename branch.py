from collections import Counter
import reader

hist = Counter()

def collect_bits(node):
  global hist
  hist[len(node.children)] += 1
  for pos, child in node.children.items():
    collect_bits(child)

def main():
  for i, node in enumerate(reader.yield_file("solution.txt")):
    if i % 100000 == 0:
      print(i)
    hist[len(node.children)] += 1
  for k in sorted(hist.keys()):
    print(k, hist[k])

if __name__ == '__main__':
  main()

