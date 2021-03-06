from flask import Flask
import reader
import reason

def read_boardvalues():
  values = [line.split() for line in open("boardvalue.hh", "rt")]
  boardvalues = {int(line[2]):line[0] for line in values}
  return boardvalues

def encode_result(result):
  return boardvalues[result].title()

def pn(proof):
  if proof == 1000000:
    return "&infin;"
  else:
    return str(proof)

def print_table(n, d, board, node, x_set, o_set, current, encode, depth):
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
          s.append('<a href="%s">%d</a><br>%s<br>%s&nbsp;-&nbsp;%s' %
              ("/game/%s%d#%d" % (current, pos, depth + 1),
               node.children[pos].count,
               encode_result(node.children[pos].result),
               pn(node.children[pos].proof), pn(node.children[pos].disproof)))
        else:
          s.append("&nbsp;")
        s.append("</td>")
      s.append("</tr>")
    s.append("</table><br><br>")
    return s

def current_player(depth):
  if depth % 2 == 0:
    return "X"
  else:
    return "O"

def print_board(n, d, board, current, node, depth):
  x_set = set(p for i, p in enumerate(board) if i % 2 == 0)
  o_set = set(p for i, p in enumerate(board) if i % 2 == 1)
  s = [('<a name="%s"><p>Result: %s, current player: %s, reason: %s, is_final: %s' +
       '<br>proof: %s disproof:%s</p></a>') % (
      depth, encode_result(node.result), current_player(depth), reasonmap[node.reason],
      node.final, pn(node.proof), pn(node.disproof))]
  if d == 2:
    s.extend(print_table(n, d, board, node, x_set, o_set,
      current, lambda i, j: j * n + i, depth))
    return "".join(s)
  elif d == 3:
    s.append("<table><tr>")
    for k in range(n):
      s.append('<td class="dim3">')
      s.extend(print_table(
        n, d, board, node, x_set, o_set,
        current, lambda i, j: i * n * n + k * n + j, depth))
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

def print_tree(tree, node, board, max_depth, depth, current, path):
  s = []
  s.extend(print_board(tree.n, tree.d, board, "".join(current), node, depth))
  if node.children and path:
    childnode = node.children[path[0]]
    board.append(path[0])
    s.extend(print_tree(
      tree, childnode, board, max_depth, depth + 1, current + [str(path[0]) + "/"], path[1:]))
    board.pop()
  return s

tree = reader.read_file("solution.txt")
reasonmap = reason.read_reason()
boardvalues = read_boardvalues()
app = Flask(__name__)
reason = reason.read_reason()
@app.route("/game/<path:subpath>")
def root(subpath):
  return render(subpath)

@app.route("/")
def rootempty():
  return render("")

def render(subpath):
  path = [int(x) for x in subpath.split("/") if x]
  s = []
  s.append(top_html())
  s.extend(print_tree(tree, tree.root, [], 5, 0, [], path))
  s.append(bottom_html())
  return "".join(s)


