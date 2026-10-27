from pathlib import Path
from collections import Counter
import json,math

r=Path(__file__).resolve().parents[1]/'Resources/AL4/Effects'
manifest=json.loads((r/'manifest.json').read_text())
for item in manifest['models']:
 p=r/item['obj'];vertices=[];normals=[];faces=[];materials=set();used=set();uv=[]
 for line in p.read_text().splitlines():
  a=line.split()
  if not a:continue
  if a[0]=='v':vertices.append(tuple(map(float,a[1:4])))
  elif a[0]=='vn':normals.append(tuple(map(float,a[1:4])))
  elif a[0]=='vt':uv.append(tuple(map(float,a[1:3])))
  elif a[0]=='f':
   assert len(a)==4,'Engine requires triangular faces'
   f=[]
   for corner in a[1:]:
    v,t,n=map(int,corner.split('/'));assert 0<v<=len(vertices) and 0<t<=len(uv) and 0<n<=len(normals)
    f.append((v-1,n-1))
   faces.append(f)
  elif a[0]=='mtllib':
   for m in (p.parent/a[1]).read_text().splitlines():
    if m.startswith('newmtl '):materials.add(m.split()[1])
  elif a[0]=='usemtl':used.add(a[1])
 assert used<=materials and materials
 assert len(vertices)==item['vertices'] and len(faces)==item['triangles']
 for axis in range(3):assert max(v[axis] for v in vertices)-min(v[axis] for v in vertices)>.01,'Mesh must have volume in all axes'
 edges=Counter();volume=0
 for f in faces:
  ids=[c[0] for c in f];a,b,c=[vertices[i] for i in ids]
  ab=[b[i]-a[i] for i in range(3)];ac=[c[i]-a[i] for i in range(3)]
  cross=(ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0])
  assert sum(v*v for v in cross)>1e-8
  n=normals[f[0][1]];assert sum(cross[i]*n[i] for i in range(3))>0,'Normals must face outwards'
  volume+=sum(a[i]*(b[(i+1)%3]*c[(i+2)%3]-b[(i+2)%3]*c[(i+1)%3]) for i in range(3))/6
  for i in range(3):edges[tuple(sorted((ids[i],ids[(i+1)%3])))]+=1
 assert all(count==2 for count in edges.values()),'Meshes must be closed'
 assert abs(volume)>1e-4,'Flat objects are not acceptable for these effects'
 print(f'PASS {p.name}: {len(vertices)} vertices, {len(faces)} triangles, closed volume {abs(volume):.4f}')
assert (r/manifest['source']).is_file()
print('All OBJ/MTL files and Blender source validated')
