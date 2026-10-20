import bpy,sys,math,struct,json
from pathlib import Path
from mathutils import Matrix
args=sys.argv[sys.argv.index('--')+1:]
if len(args)<3: raise RuntimeError('Pass GAME_DIR EXTRACTED_ASSETS OUTPUT_DIR after --')
game,assets,out=map(Path,args[:3]);out.mkdir(parents=True,exist_ok=True)
sys.path.insert(0,r'C:\Users\Alvin\AppData\Roaming\Blender Foundation\Blender\5.2\extensions\blender_org')
import dragonff;dragonff.register()
from dragonff.ops.dff_importer import import_dff
from dragonff.ops.txd_importer import import_txd
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
extra=list(args[3:]);shared=game/'models/generic/vehicle.txd'
if '--shared-txd' in extra:
 i=extra.index('--shared-txd');shared=Path(extra[i+1]);del extra[i:i+2]
report={};names=tuple(extra) if len(args)>3 else ('sentinel','infernus','banshee','landstal','bravura','buffalo','cheetah','comet','elegant','sultan','elegy')
for car in names:
 before=set(bpy.data.objects)
 images=dict(import_txd({'file_name':str(shared),'skip_mipmaps':True,'pack':True}).images)
 images.update(dict(import_txd({'file_name':str(assets/f'{car}.txd'),'skip_mipmaps':True,'pack':True}).images))
 import_dff({'file_name':str(assets/f'{car}.dff'),'txd_images':images,'image_ext':'png','connect_bones':False,'use_mat_split':True,'remove_doubles':False,'create_backfaces':False,'group_materials':False,'import_normals':True,'materials_naming':'DEF','hide_damage_parts':True})
 objects=list(set(bpy.data.objects)-before)
 wheel=next((o for o in objects if o.type=='MESH' and o.name.split('.')[0]=='wheel'),None)
 if wheel and sum(o.type=='MESH' and o.name.split('.')[0]=='wheel' for o in objects)==1:
  if wheel.parent and '_r' in wheel.parent.name:wheel.rotation_euler.z=math.pi
  for n in ('wheel_rf_dummy','wheel_lf_dummy','wheel_lb_dummy','wheel_rb_dummy'):
   if wheel.parent and wheel.parent.name.split('.')[0]==n:continue
   empty=next((o for o in objects if o.name.split('.')[0]==n),None)
   if empty:
    w=wheel.copy();w.data=wheel.data.copy();bpy.context.collection.objects.link(w);w.parent=empty;w.matrix_parent_inverse=Matrix.Identity(4);w.matrix_basis=Matrix.Identity(4)
    if '_r' in n:w.rotation_euler.z=math.pi
    objects.append(w)
 bpy.context.view_layer.update();batches=[];vertex_count=0
 for obj in objects:
  if obj.type!='MESH' or obj.hide_get() or obj.hide_render or '_dam' in obj.name or '_vlo' in obj.name:continue
  mesh=obj.data;mesh.calc_loop_triangles();uv=mesh.uv_layers.active.data if mesh.uv_layers.active else None
  for mid,mat in enumerate(mesh.materials):
   if not mat:continue
   col=list(mat.diffuse_color);nodes=mat.node_tree.nodes if mat.use_nodes else []
   p=next((n for n in nodes if n.type=='BSDF_PRINCIPLED'),None)
   if p:col=list(p.inputs['Base Color'].default_value)
   rgba=[min(255,max(0,round(c*255))) for c in col];rgb=(rgba[0]<<16)|(rgba[1]<<8)|rgba[2];paint=-1
   if rgb in (0xFFAF00,0x00FFC8,0xB9FF00,0xFF3C00):rgb=0xFFFFFF
   for i,c in enumerate((0x3CFF00,0xFF00AF,0x00FFFF,0xFF00FF)):
    if rgb==c:paint=i
   tex=next((n.image for n in nodes if n.type=='TEX_IMAGE' and n.image),None);texfile=''
   if tex and tex.size[0]>0 and tex.size[1]>0:
    texfile=car+'_'+''.join(c if c.isalnum() else '_' for c in tex.name)+'.rgba';pix=list(tex.pixels);width,height=tex.size
    data=bytearray(width*height*4)
    for y in range(height):
     for x in range(width):
      q=((height-1-y)*width+x)*4;k=(y*width+x)*4
      data[k:k+4]=bytes(min(255,max(0,round(pix[q+i]*255))) for i in (2,1,0,3))
    (out/texfile).write_bytes(struct.pack('<II',width,height)+data)
   vv=[]
   for t in mesh.loop_triangles:
    if t.material_index!=mid:continue
    coords=[obj.matrix_world@mesh.vertices[i].co for i in t.vertices];normal=(coords[1]-coords[0]).cross(coords[2]-coords[0]).normalized()
    for k,co in enumerate(coords):
     tx=uv[t.loops[k]].uv if uv else (0,0);smooth=(obj.matrix_world.to_3x3()@mesh.vertices[t.vertices[k]].normal).normalized();vv.append((*co,*smooth,tx[0],1-tx[1]))
   if vv:batches.append((paint,rgb,col[3],texfile,vv));vertex_count+=len(vv)
 with (out/f'{car}.gmesh').open('wb') as f:
  f.write(b'GTS1');f.write(struct.pack('<I',len(batches)))
  for paint,rgb,alpha,tex,vv in batches:
   f.write(struct.pack('<iIfI',paint,rgb,alpha,len(vv)));tb=tex.encode();f.write(struct.pack('<I',len(tb)));f.write(tb)
   for v in vv:f.write(struct.pack('<8f',*v))
 report[car]={'batches':len(batches),'vertices':vertex_count}
 for obj in objects:
  if obj.name in bpy.data.objects:bpy.data.objects.remove(obj,do_unlink=True)
 print('MESH_EXPORTED',car,vertex_count,flush=True)
# The editable studio asset, placed far away, mirrors the native renderer's mesh.
bpy.ops.mesh.primitive_plane_add(size=100,location=(100000,100000,1000));bpy.context.object.name='Independent showroom floor - no SA map geometry'
verts=[];faces=[]
for y in range(41):
 yy=-8+y*.8;z=max(0,(yy-9)/15)**2*14
 for x in range(49):verts.append((100000+(x-24)*2.6,100000+yy,1000+z))
for y in range(40):
 for x in range(48):i=y*49+x;faces.append((i,i+1,i+50,i+49))
m=bpy.data.meshes.new('Cyclorama');m.from_pydata(verts,[],faces);o=bpy.data.objects.new('Curved studio backdrop',m);bpy.context.collection.objects.link(o)
bpy.ops.wm.save_as_mainfile(filepath=str(out/'studio.blend'))
(out/'assets.json').write_text(json.dumps(report,indent=2));print('EXPORT_COMPLETE',flush=True)
