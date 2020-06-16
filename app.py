from flask import Flask
import reader
 
def encode_result(value):
  d = {0: "X wins", 1: "O wins", 2: "draw"}
  return d[value]

def print_table(n, d, board, node, x_set, o_set, encode):
    s = []
    s.append('<table class="border board">')
    children = node.children.keys()
    for j in range(0, n):
      s.append('<tr class="border">')
      for i in range(0, n):
        s.append('<td class="border cell">')
        pos = encode(i, j)
        if pos in x_set:
          s.append("X")
        elif pos in o_set:
          s.append("O")
        elif pos in children:
          s.append('<a href="sffdf">%d</a><br>(%s)' %
              (node.children[pos].count, encode_result(node.children[pos].result)))
        else:
          s.append("&nbsp;")
        s.append("</td>")
      s.append("</tr>")
    s.append("</table><br><br>")
    return s

def print_board(n, d, board, node):
  x_set = set(p for i, p in enumerate(board) if i % 2 == 0)
  o_set = set(p for i, p in enumerate(board) if i % 2 == 1)
  s = ['<p>Result: %s</p>' % encode_result(node.result)]
  if d == 2:
    s.extend(print_table(n, d, board, node, x_set, o_set, lambda i, j: j * n + i))
    return "".join(s)
  elif d == 3:
    s.append("<table><tr>")
    for k in range(n):
      s.append('<td class="dim3">')
      s.extend(print_table(
        n, d, board, node, x_set, o_set, lambda i, j: k * n * n + j * n + i))
      s.append("</td>")
    s.append("</tr></table>")
    return "".join(s)

def top_html():
  return """
  <html>
  <head>
  <style>
    .board {
      border-collapse: collapse;
    }
    .border {
      border: 1px solid black;
    }
    .cell {
      width: 60px;
      height: 60px;
      text-align: center;
    }
    .dim3 {
      padding: 20px;
    }
  </style>
  </head>
  <body>"""

def bottom_html():
  return """
  </body>
  </html>
  """

def print_tree(tree, node, board, max_depth, depth):
  s = []
  s.extend(print_board(tree.n, tree.d, board, node))
  if depth < max_depth:
    for pos, childnode in node.children.items():
      board.append(pos)
      s.extend(print_tree(tree, childnode, board, max_depth, depth + 1))
      board.pop()
  return s

app = Flask(__name__)
@app.route("/")
def root():
  tree = reader.read_file("solution.txt")
  s = []
  s.append(top_html())
  s.extend(print_tree(tree, tree.root, [], 5, 0))
  s.append(bottom_html())
  return "".join(s)


