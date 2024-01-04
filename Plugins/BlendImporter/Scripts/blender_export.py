import sys, os, bpy
from mathutils import Vector, Matrix

# Functions

def FindBSDFNode(mat):
    if not mat.use_nodes:
        return None
    
    for node in mat.node_tree.nodes:
        if node.type == "BSDF_PRINCIPLED":
            return node
    
    return None

def FindLinkedTextureImageNode(node, initialInput):
    if node.type == "TEX_IMAGE":
        return node
    else:
        input = None
        if initialInput:
            input = initialInput
        elif "Color" in node.inputs:
            input = node.inputs["Color"]
        
        if input and input.is_linked:
            return FindLinkedTextureImageNode(input.links[0].from_node, None)
        else:
            return None

def FixMaterials():
    for mat in bpy.data.materials:
        BSDFNode = FindBSDFNode(mat)
        if BSDFNode:
            # try to find texture node and link directly to Base Color
            baseColorTextureNode = FindLinkedTextureImageNode(BSDFNode, BSDFNode.inputs["Base Color"])
            if baseColorTextureNode is not None:
                mat.node_tree.links.new(baseColorTextureNode.outputs["Color"], BSDFNode.inputs["Base Color"])
            
            # switch roughness to specular
            roughnessTextureNode = FindLinkedTextureImageNode(BSDFNode, BSDFNode.inputs["Roughness"])
            if roughnessTextureNode is not None:
                specularInputName = "Specular"
                # since Blender 4.0, Specular input is now called "Specular IOR Level"
                if specularInputName not in BSDFNode.inputs:
                    specularInputName = "Specular IOR Level"
                mat.node_tree.links.new(roughnessTextureNode.outputs["Color"], BSDFNode.inputs[specularInputName])
                
            # switch metallic to roughness and clear metallic
            metallicTextureNode = FindLinkedTextureImageNode(BSDFNode, BSDFNode.inputs["Metallic"])
            if metallicTextureNode is not None:
                mat.node_tree.links.new(metallicTextureNode.outputs["Color"], BSDFNode.inputs["Roughness"])
                mat.node_tree.links.remove(BSDFNode.inputs["Metallic"].links[0])
                

def RecurseGatherMeshChildren(target):
    allChildren = [] 
    for ob in bpy.data.objects: 
        if ob.type == "MESH" and ob.parent == target:
            allChildren.append(ob)
            children = RecurseGatherMeshChildren(ob)
            allChildren += children
    return allChildren

def JoinMeshChildren(target):
    children = RecurseGatherMeshChildren(target)
    if len(children) == 0:
        return
    
    if debug:
        print(f"Joining {len(children)} children to {target.name}")
    
    list = [ target ]
    list = list + children
    
    ctx = bpy.context.copy()
    ctx['active_object'] = list[0]
    ctx['selected_objects'] = list
    ctx['selected_editable_objects'] = list

    bpy.ops.object.join(ctx)

# Main

outfile = os.getenv("UNREAL_IMPORTER_OUTPUT_FILE")
set_object_pivot = (os.getenv("UNREAL_IMPORTER_EXPORT_OBJECT_PIVOT") == 'true')
combine_child_meshes = (os.getenv("UNREAL_IMPORTER_COMBINE_CHILD_MESHES") == 'true')
fix_materials = (os.getenv("UNREAL_IMPORTER_FIX_MATERIALS") == 'true')
unpack = (os.getenv("UNREAL_IMPORTER_UNPACK") == 'true')
debug = "-d" in sys.argv

if debug:
    import addon_utils
    print("Enabled Addons:")
    for mod in addon_utils.modules():
        modName = mod.bl_info.get('name')
        modVersion = mod.bl_info.get('version', (-1, -1, -1))
        default, state = addon_utils.check(mod.__name__)
        if default:
            print(f" - {modName} ({modVersion})")
    
if outfile is None:
    outfile = bpy.data.filepath + ".fbx"

enabled_collections = os.getenv("UNREAL_IMPORTER_ENABLED_COLLECTIONS")
if enabled_collections == "":
    enabled_collections = None
if enabled_collections is not None:
    enabled_collections = enabled_collections.split(",")

if debug:
    print ("OutFile: " + outfile)
    print ("Debug: " + str(debug))
    print ("Set Object Pivot: " + str(set_object_pivot))
    print ("Combine Child Meshes: " + str(combine_child_meshes))
    print ("Fix Materials: " + str(fix_materials))
    print ("Unpack: " + str(unpack))
    print ("Enabled Collections: " + str(enabled_collections))

if fix_materials:
    FixMaterials()

# Deselect all objects and ensure we are in object mode. Otherwise already-selected objects will be included in the export. (Thanks to https://twitter.com/NeoFutureLabs for help with this!)
for obj in bpy.context.selected_objects:
    obj.select_set(False)

if enabled_collections:
    for collection in enabled_collections:
        for obj in bpy.data.collections[collection].all_objects:
            if obj.visible_get():
                obj.select_set(True)
else:
    for obj in bpy.data.objects:
        if obj.visible_get():
            obj.select_set(True)

if combine_child_meshes:
    root_meshes = [o for o in bpy.context.scene.objects if o.type == "MESH" and o.select_get() and (o.parent == None or o.parent.type != "MESH")]
    for mesh in root_meshes:
        JoinMeshChildren(mesh)

if set_object_pivot:
    for obj in bpy.context.scene.objects:
        if combine_child_meshes and obj.parent:
            continue
        
        if obj.select_get():
            if debug:
                print(f"Set object pivot for {obj.name}")
            obj.location = Vector()
            obj.matrix_world = Matrix()

path_mode="AUTO"
embed_textures=False

if unpack:
    path_mode="COPY"
    embed_textures=True

bpy.ops.export_scene.fbx(filepath=outfile,
    axis_forward='-Z',
    axis_up='Y',
    check_existing=False,
    object_types={'ARMATURE','CAMERA','LIGHT','MESH','OTHER','EMPTY'},
    mesh_smooth_type='FACE', # This prevents a warning about undefined smoothing groups in Unreal
    use_selection=True,
    use_custom_props=True,
    apply_scale_options='FBX_SCALE_NONE',
    bake_anim_use_nla_strips=True,
    bake_anim_use_all_actions=True,
    add_leaf_bones=False,
    use_armature_deform_only=False,
    path_mode=path_mode,
    embed_textures=embed_textures)

print ("Export Complete")

if not bpy.app.background:
    bpy.ops.wm.quit_blender()