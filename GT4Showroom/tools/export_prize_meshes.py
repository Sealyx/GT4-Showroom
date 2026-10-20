import bpy,sys,json,struct,math
from pathlib import Path
from mathutils import Matrix
args=sys.argv[sys.argv.index('--')+1:]
assets,out,plans_file,game=map(Path,args[:4]);out.mkdir(parents=True,exist_ok=True)
sys.path.insert(0,r'C:\Users\Alvin\AppData\Roaming\Blender Foundation\Blender\5.2\extensions\blender_org')
import dragonff;dragonff.register()
from dragonff.ops.dff_importer import import_dff
from dragonff.ops.txd_importer import import_txd
plans=json.loads(plans_file.read_text(encoding='utf-8-sig'))
parts={row['id']:row for row in json.loads((assets/'manifest.json').read_text())['parts']}
vehicle_rows={}
for line in (game/'data/vehicles.ide').read_text().splitlines():
 t=[x.strip() for x in line.split(',')]
 if len(t)>=14 and t[0].isdigit():vehicle_rows[t[1]]=t
report={}
def load_dff(name,images):
 before=set(bpy.data.objects)
 import_dff({'file_name':str(assets/(name+'.dff')),'txd_images':images,'image_ext':'png','connect_bones':False,'use_mat_split':False,'remove_doubles':False,'create_backfaces':False,'group_materials':False,'import_normals':True,'materials_naming':'DEF','hide_damage_parts':True})
 return list(set(bpy.data.objects)-before)
for plan in plans:
 for obj in list(bpy.data.objects):bpy.data.objects.remove(obj,do_unlink=True)
 car=plan['model'];images={}
 for txd in [game/'models/generic/vehicle.txd',assets/(car+'.txd')]:images.update(dict(import_txd({'file_name':str(txd),'skip_mipmaps':True,'pack':True}).images))
 remaps=dict(import_txd({'file_name':str(assets/f'{car}{plan["paintjob"]+1}.txd'),'skip_mipmaps':True,'pack':True}).images)
 remap=next(iter(remaps.values()))[0];remap_originals=set(image for key,mips in images.items() if key.startswith('#') or 'body' in key.lower() for image in mips)
 objects=load_dff(car,images);bpy.context.view_layer.update();anchors={o.name.split('.')[0]:o.matrix_world.copy() for o in objects}
 # Replace bumpers/exhaust/wheels; native replacement atomics discard asset-frame transforms.
 meshes=[o for o in objects if o.type=='MESH' and not o.hide_get() and not o.hide_render and not any(tag in o.name.lower() for tag in ('_dam','_vlo','.col','shadowmesh')) and o.name.split('.')[0] not in ('bump_front_ok','bump_rear_ok','exhaust_ok','wheel')]
 all_parts=plan['parts'][:6]+[parts[plan['parts'][5]]['id']]
 skirt=parts[plan['parts'][5]]['model'];right=next(row for row in parts.values() if row['model']==skirt.replace('wg_l_','wg_r_'))
 all_parts=plan['parts'][:6]+[right['id'],plan['wheel']]
 placement=[]
 for id in all_parts:
  info=parts[id];name=info['model'];part_images=dict(images)
  txd=assets/(info['txd']+'.txd')
  if info['txd']!='vehicle' and txd.exists():part_images.update(dict(import_txd({'file_name':str(txd),'skip_mipmaps':True,'pack':True}).images))
  new=load_dff(name,part_images);good=[o for o in new if o.type=='MESH' and '_dam' not in o.name and not o.hide_get()]
  wheel=name.startswith('wheel_')
  anchor='bump_front_dummy' if name.startswith('fbmp_') else 'bump_rear_dummy' if name.startswith('rbmp_') else 'ug_roof' if name.startswith('rf_') else 'ug_spoiler' if name.startswith('spl_') else 'ug_wing_left' if name.startswith('wg_l_') else 'ug_wing_right' if name.startswith('wg_r_') else 'exhaust_ok'
  targets=['wheel_rf_dummy','wheel_lf_dummy','wheel_rb_dummy','wheel_lb_dummy'] if wheel else [anchor]
  for target in targets:
   assert target in anchors,(car,target)
   transform=anchors[target].copy()
   if wheel:
    size=float(vehicle_rows[car][12 if target[7]=='f' else 13])
    # Match the four ordinary rest-wheel poses in the native preview adapter.
    rotation=Matrix.Rotation(math.pi,4,'Z') if target.startswith('wheel_r') else Matrix.Identity(4)
    transform=transform@rotation@Matrix.Diagonal((size,size,size,1))
   for source in good:
    clone=source.copy();clone.data=source.data.copy();bpy.context.collection.objects.link(clone);clone.parent=None;clone.matrix_world=transform;meshes.append(clone)
   placement.append({'part':name,'anchor':target,'matrix':[list(row) for row in transform]})
 bpy.context.view_layer.update();batches=[];texture_cache={}
 for obj in meshes:
  mesh=obj.data;mesh.calc_loop_triangles();uv=mesh.uv_layers.active.data if mesh.uv_layers.active else None
  for mid,mat in enumerate(mesh.materials):
   if not mat:continue
   nodes=mat.node_tree.nodes if mat.use_nodes else [];shader=next((n for n in nodes if n.type=='BSDF_PRINCIPLED'),None);col=list(shader.inputs['Base Color'].default_value if shader else mat.diffuse_color)
   rgba=[min(255,max(0,round(c*255))) for c in col];rgb=(rgba[0]<<16)|(rgba[1]<<8)|rgba[2];paint=-1
   if rgb in (0xFFAF00,0x00FFC8,0xB9FF00,0xFF3C00):rgb=0xFFFFFF
   for i,color in enumerate((0x3CFF00,0xFF00AF,0x00FFFF,0xFF00FF)):
    if rgb==color:paint=i
   image=next((n.image for n in nodes if n.type=='TEX_IMAGE' and n.image),None)
   if image in remap_originals:image=remap
   if paint==0:paint=-1;rgb=0xFFFFFF # native paintjob sets primary colour to white
   texture=''
   if image and image.size[0] and image.size[1]:
    if image.name not in texture_cache:
     filename=car+'_prize_'+''.join(c if c.isalnum() else '_' for c in image.name)+'.rgba';width,height=image.size;pix=list(image.pixels);data=bytearray(width*height*4)
     for y in range(height):
      for x in range(width):
       q=((height-1-y)*width+x)*4;k=(y*width+x)*4;data[k:k+4]=bytes(min(255,max(0,round(pix[q+i]*255))) for i in (2,1,0,3))
     (out/filename).write_bytes(struct.pack('<II',width,height)+data);texture_cache[image.name]=filename
    texture=texture_cache[image.name]
   vertices=[]
   for tri in mesh.loop_triangles:
    if tri.material_index!=mid:continue
    for k,index in enumerate(tri.vertices):
     pos=obj.matrix_world@mesh.vertices[index].co;normal=(obj.matrix_world.to_3x3()@mesh.vertices[index].normal).normalized();tx=uv[tri.loops[k]].uv if uv else (0,0);vertices.append((*pos,*normal,tx[0],1-tx[1]))
   if vertices:batches.append((paint,rgb,col[3],texture,vertices))
 with (out/(car+'_prize.gmesh')).open('wb') as f:
  f.write(b'GTS1');f.write(struct.pack('<I',len(batches)))
  for paint,rgb,alpha,tex,vertices in batches:
   f.write(struct.pack('<iIfI',paint,rgb,alpha,len(vertices)));encoded=tex.encode();f.write(struct.pack('<I',len(encoded)));f.write(encoded)
   for vertex in vertices:f.write(struct.pack('<8f',*vertex))
 report[car]={'plan':plan,'batches':len(batches),'triangles':sum(len(row[4])//3 for row in batches),'placements':placement,'remap':remap.name}
 print('PRIZE_EXPORTED',car,report[car]['triangles'],flush=True)
 # One editable sample per car for reviewing placement without launching GTA.
 visible=set(meshes)
 for obj in meshes:
  world=obj.matrix_world.copy();obj.parent=None;obj.matrix_world=world
 for obj in list(bpy.data.objects):
  if obj not in visible:bpy.data.objects.remove(obj,do_unlink=True)
 bpy.ops.wm.save_as_mainfile(filepath=str(out/(car+'_prize.blend')))
(out/'prize-assets.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('PRIZE_EXPORT_COMPLETE',flush=True)
