//Maya ASCII 2024 scene
//Name: test_scene_sample.ma
//Last modified: Tue, Jan 14, 2025 02:00:00 PM
//Codeset: UTF-8
requires maya "2024";
requires "mtoa" "5.3.4.1";
currentUnit -l centimeter -a degree -t film;
fileInfo "application" "maya";
fileInfo "product" "Maya 2024";
fileInfo "version" "2024";
fileInfo "cutIdentifier" "202310181224-69282f2959";
fileInfo "osv" "Windows 11 Home v2009 (Build: 22621)";
fileInfo "UUID" "12345678-1234-1234-1234-123456789ABC";

createNode transform -s -n "persp";
	rename -uid "ABCD1234-1234-1234-1234-123456789ABC";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 28.0 21.0 28.0;
	setAttr ".r" -type "double3" -27.938352729602379 44.999999999999972 0;

createNode camera -s -n "perspShape" -p "persp";
	rename -uid "EFGH5678-5678-5678-5678-567890123456";
	setAttr -k off ".v" no;
	setAttr ".fl" 34.999999999999993;
	setAttr ".coi" 44.82186966202994;
	setAttr ".imn" -type "string" "persp";
	setAttr ".den" -type "string" "persp_depth";
	setAttr ".man" -type "string" "persp_mask";
	setAttr ".hc" -type "string" "viewSet -p %camera";

// 材质节点
createNode shadingEngine -n "aiStandardSurface1SG";
	rename -uid "IJKL9012-9012-9012-9012-901234567890";
	setAttr ".ihi" 0;
	setAttr ".ro" yes;

createNode materialInfo -n "materialInfo1";
	rename -uid "MNOP3456-3456-3456-3456-345678901234";

createNode aiStandardSurface -n "aiStandardSurface1";
	rename -uid "QRST7890-7890-7890-7890-789012345678";

// 纹理文件节点
createNode file -n "file1";
	rename -uid "UVWX1234-1234-1234-1234-123456789012";
	setAttr ".ftn" -type "string" "C:/Projects/Textures/wood_diffuse.jpg";
	setAttr ".cs" -type "string" "sRGB";

createNode file -n "file2";
	rename -uid "YZAB5678-5678-5678-5678-567890123456";
	setAttr ".ftn" -type "string" "C:/Projects/Textures/wood_normal.png";
	setAttr ".cs" -type "string" "Raw";

createNode file -n "file3";
	rename -uid "CDEF9012-9012-9012-9012-901234567890";
	setAttr ".ftn" -type "string" "D:/Assets/Textures/metal_roughness.exr";
	setAttr ".cs" -type "string" "Raw";

// Arnold 灯光
createNode aiAreaLight -n "aiAreaLightShape1";
	rename -uid "GHIJ3456-3456-3456-3456-345678901234";
	setAttr -k off ".v";
	setAttr ".csh" no;
	setAttr ".rcsh" no;
	setAttr ".ai_exposure" 8;

// IES 文件
createNode aiPhotometricLight -n "aiPhotometricLight1";
	rename -uid "KLMN7890-7890-7890-7890-789012345678";
	setAttr ".iesProfile" -type "string" "C:/Projects/Lights/studio_light.ies";

// Alembic 缓存
createNode AlembicNode -n "AlembicNode1";
	rename -uid "OPQR1234-1234-1234-1234-123456789012";
	setAttr ".abc_File" -type "string" "D:/Cache/character_animation.abc";

// GPU Cache
createNode gpuCache -n "gpuCache1";
	rename -uid "STUV5678-5678-5678-5678-567890123456";
	setAttr ".cfn" -type "string" "C:/Cache/environment.abc";

// 关系连接
connectAttr "file1.oc" "aiStandardSurface1.base_color";
connectAttr "file2.oc" "aiStandardSurface1.normal_camera";
connectAttr "file3.ocr" "aiStandardSurface1.specular_roughness";
connectAttr "aiStandardSurface1.out" "aiStandardSurface1SG.ss";

// 选择信息
select -ne :time1;
	setAttr ".o" 1;
	setAttr ".unw" 1;
select -ne :hardwareRenderingGlobals;
	setAttr ".otfna" -type "stringArray" 22 "NURBS Curves" "NURBS Surfaces" "Polygons" "Subdiv Surface" "Particles" "Particle Instance" "Fluids" "Strokes" "Image Planes" "UI" "Lights" "Cameras" "Locators" "Joints" "IK Handles" "Deformers" "Motion Trails" "Components" "Hair Systems" "Follicles" "Misc. UI" "Ornaments"  ;
	setAttr ".otfva" -type "Int32Array" 22 0 1 1 1 1 1
		 1 1 1 0 0 0 0 0 0 0 0 0
		 0 0 0 0 ;
	setAttr ".fprt" yes;
select -ne :renderPartition;
	setAttr -s 3 ".st";
select -ne :renderGlobalsList1;
select -ne :defaultShaderList1;
	setAttr -s 6 ".s";
select -ne :postProcessList1;
	setAttr -s 2 ".p";
select -ne :defaultRenderingList1;
select -ne :initialShadingGroup;
	setAttr -s 2 ".dsm";
	setAttr ".ro" yes;
select -ne :initialParticleSE;
	setAttr ".ro" yes;
select -ne :defaultRenderGlobals;
	addAttr -ci true -h true -sn "dss" -ln "defaultSurfaceShader" -dt "string";
	setAttr ".ren" -type "string" "arnold";
	setAttr ".dss" -type "string" "lambert1";
select -ne :defaultResolution;
	setAttr ".pa" 1;
select -ne :defaultColorMgtGlobals;
	setAttr ".cfe" yes;
	setAttr ".cfp" -type "string" "<MAYA_RESOURCES>/OCIO-configs/Maya2022-default/config.ocio";
	setAttr ".vtn" -type "string" "ACES 1.0 SDR-video (sRGB)";
	setAttr ".wsn" -type "string" "ACEScg";
	setAttr ".otn" -type "string" "ACES 1.0 SDR-video (sRGB)";
	setAttr ".potn" -type "string" "ACES 1.0 SDR-video (sRGB)";
select -ne :hardwareRenderGlobals;
	setAttr ".ctrs" 256;
	setAttr ".btrs" 512;

// 结束
