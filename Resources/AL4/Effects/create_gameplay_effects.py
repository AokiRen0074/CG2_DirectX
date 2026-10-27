import bpy,bmesh,math,json
from pathlib import Path
from mathutils import Vector

repo=Path(__file__).resolve().parents[3]
out=repo/'Resources/AL4/Effects';out.mkdir(parents=True,exist_ok=True)
preview=out/'preview.png'
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
materials=[]
for name,color in [('Facet_Light',(.95,.95,.95,1)),('Facet_Mid',(.68,.68,.68,1)),('Facet_Dark',(.38,.38,.38,1))]:
 m=bpy.data.materials.new(name);m.diffuse_color=color;materials.append(m)
cyan=bpy.data.materials.new('Refill_Cyan');cyan.diffuse_color=(.08,.72,1,1)
cyan_mid=bpy.data.materials.new('Refill_Blue');cyan_mid.diffuse_color=(.025,.32,.62,1)

def mesh_object(name,vertices,faces,mats,color):
 mesh=bpy.data.meshes.new(name);mesh.from_pydata(vertices,[],faces);mesh.update()
 bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.recalc_face_normals(bm,faces=bm.faces);bm.to_mesh(mesh);bm.free();mesh.update()
 obj=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(obj)
 for m in mats:mesh.materials.append(m)
 for p in mesh.polygons:p.material_index=p.index%len(mats);p.use_smooth=False
 obj.color=color;return obj

def pointed(name,ring_x,width,depth,sides,color,tip=1,tail=-1):
 vertices=[(tail,0,.03),(tip,0,0)]
 for i in range(sides):
  a=i*2*math.pi/sides;vertices.append((ring_x,math.cos(a)*width,math.sin(a)*depth))
 faces=[]
 for i in range(sides):
  a=2+i;b=2+(i+1)%sides;faces.extend([(0,b,a),(1,a,b)])
 return mesh_object(name,vertices,faces,materials,color)

objects=[
 pointed('effect_shard',-.12,.45,.3,4,(.12,.65,1,1)),
 pointed('crystal_projectile',-.15,.45,.35,6,(1,.38,.06,1)),
 pointed('lance_projectile',-.3,.28,.22,4,(1,.6,.15,1)),
 mesh_object('ambient_grain',[(1,0,0),(-1,0,0),(0,.7,0),(0,-.7,0),(0,0,.6),(0,0,-.6)],
             [(0,2,4),(2,1,4),(1,3,4),(3,0,4),(2,0,5),(1,2,5),(3,1,5),(0,3,5)],materials,(.4,.85,1,1)),
 mesh_object('refill_crystal',[(0,1,0),(0,-1,0),(-.52,0,0),(.52,0,0),(0,0,.32),(0,0,-.32)],
             [(0,2,4),(0,4,3),(0,3,5),(0,5,2),(1,4,2),(1,3,4),(1,5,3),(1,2,5)],[cyan,cyan_mid],(.08,.85,1,1))
]

def export_obj(obj):
 mesh=obj.data;mesh.calc_loop_triangles()
 lines=[f'# Blender-authored gameplay mesh: {obj.name}',f'mtllib {obj.name}.mtl',f'o {obj.name}']
 # Match the engine loader's X mirror and reversed triangle winding.
 for v in mesh.vertices:lines.append(f'v {-v.co.x:.8f} {v.co.y:.8f} {v.co.z:.8f}')
 lines.extend(['vt 0.0 0.0','vt 1.0 0.0','vt 0.5 1.0'])
 tris=list(mesh.loop_triangles)
 for tri in tris:
  n=tri.normal;lines.append(f'vn {-n.x:.8f} {n.y:.8f} {n.z:.8f}')
 for i,tri in enumerate(tris):
  mat=mesh.materials[tri.material_index];lines.append('usemtl '+mat.name)
  inds=tri.vertices;lines.append(f'f {inds[0]+1}/1/{i+1} {inds[2]+1}/3/{i+1} {inds[1]+1}/2/{i+1}')
 (out/(obj.name+'.obj')).write_text('\n'.join(lines)+'\n',encoding='utf-8')
 mtl=[]
 for m in mesh.materials:
  c=m.diffuse_color;mtl.extend(['newmtl '+m.name,f'Kd {c[0]:.5f} {c[1]:.5f} {c[2]:.5f}','Ka 0 0 0','Ks 0.35 0.35 0.35','Ns 48','d 1',''])
 (out/(obj.name+'.mtl')).write_text('\n'.join(mtl),encoding='utf-8')
 return {'name':obj.name,'vertices':len(mesh.vertices),'triangles':len(tris),'obj':obj.name+'.obj','mtl':obj.name+'.mtl'}

manifest=[export_obj(o) for o in objects]
(out/'manifest.json').write_text(json.dumps({'source':'GameplayEffects.blend','models':manifest},indent=2)+'\n',encoding='utf-8')
for i,obj in enumerate(objects):obj.location=(i*2.7,0,0);obj.rotation_euler=(.15,0,.2)

scene=bpy.context.scene
scene.render.engine='BLENDER_WORKBENCH'
scene.display.shading.light='STUDIO';scene.display.shading.color_type='OBJECT'
scene.display.shading.show_shadows=True;scene.display.shading.show_cavity=True
scene.display.shading.cavity_type='BOTH';scene.display.shading.background_type='WORLD'
scene.world.color=(.018,.025,.04)
bpy.ops.object.camera_add(location=(6,-11,8));camera=bpy.context.object;camera.name='Asset_Preview'
camera.rotation_euler=(Vector((5.4,0,0))-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type='ORTHO';camera.data.ortho_scale=14;scene.camera=camera
scene.render.resolution_x=1400;scene.render.resolution_y=600;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(preview)
bpy.ops.wm.save_as_mainfile(filepath=str(out/'GameplayEffects.blend'))
bpy.ops.render.render(write_still=True)
print('OBJ_ASSETS_EXPORTED '+json.dumps(manifest))
