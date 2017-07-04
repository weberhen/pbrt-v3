#include "makescene.h"

namespace ns {
    // a simple struct to model a person
    struct scene {
        std::string camera;
        std::vector<std::vector< pbrt::Float> > objectPose;
        std::vector<std::vector<float> > cameraLookat;
        std::vector<float> fov;   
        std::string bsdf;
        std::string envmap;
        std::string mesh;
        std::string vpls;
        int xresolution;
        int yresolution;
    };

    void from_json(const json& j, scene& s) {
        if(j.find("camera") != j.end())
            s.camera = j.at("camera").get<std::string>();
        if(j.find("objectPose") != j.end())
            s.objectPose = j.at("objectPose").get<std::vector< std::vector<float> > >();
        if(j.find("cameraLookat") != j.end())
            s.cameraLookat = j.at("cameraLookat").get<std::vector< std::vector<float> > >();        
        if(j.find("fov") != j.end())            
            s.fov = j.at("fov").get<std::vector<float> >();
        if(j.find("bsdf") != j.end())
            s.bsdf = j.at("bsdf").get<std::string >();
        if(j.find("envmap") != j.end())
            s.envmap = j.at("envmap").get<std::string >();
        if(j.find("mesh") != j.end())
            s.mesh = j.at("mesh").get<std::string >();
        if(j.find("vpls") != j.end())
            s.vpls = j.at("vpls").get<std::string >();
        if(j.find("xresolution") != j.end())
            s.xresolution = j.at("xresolution").get<int>();
        if(j.find("yresolution") != j.end())
            s.yresolution = j.at("yresolution").get<int>();
    };
}

namespace pbrt {

struct ParamArray {
    ParamArray() {
        isString = false;
        element_size = allocated = nelems = 0;
        array = nullptr;
    }
    bool isString;
    int element_size;
    int allocated;
    int nelems;
    void *array;
};

struct ParamListItem {
    ParamListItem(const char *t, ParamArray *array) {
        arg = array->array;
        name = t;
        size = array->nelems;
        isString = array->isString;
        array->allocated = 0;
        array->nelems = 0;
        array->array = nullptr;
    }
    const char *name;
    void *arg;
    int size;
    bool isString;
};

struct vpls {
    double pos[3]; // 4.644200 0.000000 -1.963826
    double nor[3]; // -0.886482 0.264871 0.379464
    double col[3]; // 0.000260 0.000143 0.000094
    double scale; // 27.385206
};

static std::vector<ParamListItem> cur_paramlist;
static ParamArray *cur_array = nullptr;

enum { PARAM_TYPE_INT, PARAM_TYPE_BOOL, PARAM_TYPE_FLOAT,
    PARAM_TYPE_POINT2, PARAM_TYPE_VECTOR2, PARAM_TYPE_POINT3,
    PARAM_TYPE_VECTOR3, PARAM_TYPE_NORMAL, PARAM_TYPE_RGB, PARAM_TYPE_XYZ,
    PARAM_TYPE_BLACKBODY, PARAM_TYPE_SPECTRUM,
    PARAM_TYPE_STRING, PARAM_TYPE_TEXTURE };
static const char *paramTypeToName(int type);
static bool lookupType(const char *name, int *type, std::string &sname);
static void InitParamSet(ParamSet &ps, SpectrumType);

void MakeScene(std::string filename)
{
    std::ifstream i(filename);
    json j;
    i >> j;
    ns::scene s = j;
    int NVPLS = 0;
    vpls *p;
    
    for(int ix = 0; ix<j["nScenes"];ix++)
    {
        pbrt::ParamSet params;
        pbrt::InitParamSet(params, pbrt::SpectrumType::Reflectance);
        
        //LookAt .5 0 .4   0 0 0 0 0 1
        //std::vector<float> op = j["objectPoses"];
        pbrt::pbrtLookAt(s.cameraLookat[ix]);

        //Camera "perspective" "float fov" [45]
        std::unique_ptr<Float[]> fov(new Float[1]);
        fov[0] = s.fov[ix];
        params.AddFloat("fov",std::move(fov),1);
        
        pbrt::pbrtCamera(s.camera, params);

        //Sampler "sobol" "integer pixelsamples" [4]
        pbrt::InitParamSet(params, pbrt::SpectrumType::Reflectance);
        std::unique_ptr<int[]> pixelsamples(new int[1]);
        pixelsamples[0] = 1;
        params.AddInt("pixelsamples",std::move(pixelsamples),1);
        pbrt::pbrtSampler("sobol", params);

        //Integrator "volpath" "integer maxdepth" [25]
        pbrt::InitParamSet(params, pbrt::SpectrumType::Reflectance);
        std::unique_ptr<int[]> maxdepth(new int[1]);
        maxdepth[0] = 1;
        params.AddInt("maxdepth",std::move(maxdepth),1);
        pbrt::pbrtIntegrator("path", params);

        //Film "image" "string filename" ["green-acrylic-bunny.png"]
        pbrt::InitParamSet(params, pbrt::SpectrumType::Reflectance);
        std::unique_ptr<std::string[]> filename(new std::string[1]);
        filename[0] = std::to_string(ix) + ".exr";
        params.AddString("filename",std::move(filename),1);
        
        //     "integer xresolution" [200] "integer yresolution" [200]
        std::unique_ptr<int[]> xresolution(new int[1]);
        xresolution[0] = s.xresolution;
        params.AddInt("xresolution",std::move(xresolution),1);
        std::unique_ptr<int[]> yresolution(new int[1]);
        yresolution[0] = s.yresolution;
        params.AddInt("yresolution",std::move(yresolution),1);  
        pbrt::pbrtFilm("image", params);

        //WorldBegin
        pbrt::pbrtWorldBegin();

        //  CoordSysTransform "camera"
        //pbrt::pbrtCoordSysTransform("camera");
                
        if(!s.envmap.empty()){

            //AttributeBegin
            pbrt::pbrtAttributeBegin();

            pbrt::InitParamSet(params, pbrt::SpectrumType::Illuminant);
            //  "string mapname" "envmaps/no_hole_9C4A0307_Panorama_hdr.exr"
            std::unique_ptr<std::string[]> mapname(new std::string[1]);
            mapname[0] = s.envmap;
            params.AddString("mapname",std::move(mapname),1);
            //LightSource "infinite"
            pbrt::pbrtRotate(90,-1,0,0);
            pbrt::pbrtLightSource("infinite", params);

            //AttributeEnd
            pbrt::pbrtAttributeEnd();
        }   
        
        if(!s.vpls.empty()){

            std::string line;
            std::ifstream vpls_data;
            vpls_data.open(s.vpls);
            
            std::istringstream iss;
            std::string substring;

            if (vpls_data.is_open()) 
            {
                std::getline (vpls_data,line); //skips first line of data_vpls.txt
                

                iss.str(line);
                iss >> substring; iss >> substring; 
                int step = 1;
                NVPLS = std::stoi(substring)/step;

/*                for(int v=0;v<2294272;v++)
                        std::getline (vpls_data,line);
*/
                p = (struct vpls*)  malloc(NVPLS * sizeof(struct vpls)); 
                for(int j=0;j<NVPLS;j++)
                {

                    /*for(int v=0;v<step*4;v++)
                        std::getline (vpls_data,line);*/

                    std::getline (vpls_data,line);
                    //std::cout<<line<<std::endl;
                    iss.str(line);
                    //position
                    iss >> substring;
                    for (int i=0;i<3;i++)
                    {
                        iss >> substring;
                        p[j].pos[i] = std::stod(substring);
                    }
                    
                    //normal
                    std::getline (vpls_data,line);  
                    iss.str(line);
                    iss >> substring;
                    for (int i=0;i<3;i++)
                    {
                        iss >> substring;
                        p[j].nor[i] = std::stod(substring);
                    }

                    //color
                    std::getline (vpls_data,line);  
                    iss.str(line);
                    iss >> substring;
                    for (int i=0;i<3;i++)
                    {
                        iss >> substring;
                        p[j].col[i] = std::stod(substring);
                    }

                    //scale
                    std::getline (vpls_data,line);
                    iss.str(line);
                    iss >> substring;
                    iss >> substring;
                    p[j].scale = std::stod(substring);
                    
                    /*std::cout<<"pos: "<<p[j].pos[0]<<" "<<p[j].pos[1]<<" "<<p[j].pos[2]<<" "<<std::endl;
                    std::cout<<"nor: "<<p[j].nor[0]<<" "<<p[j].nor[1]<<" "<<p[j].nor[2]<<" "<<std::endl;
                    std::cout<<"col: "<<p[j].col[0]<<" "<<p[j].col[1]<<" "<<p[j].col[2]<<" "<<std::endl;
                    std::cout<<"sca: "<<p[j].scale<<std::endl;
                    char a;
                    scanf("%c",&a);*/
                    
                }
                vpls_data.close();
                
            }
            else
            {
                std::cout<<"Error reading vpls_data.txt"<<std::endl;
            }

            for(int j=0;j<NVPLS;j++)
            {
                //AttributeBegin
                pbrt::pbrtAttributeBegin();
                pbrt::InitParamSet(params, pbrt::SpectrumType::Illuminant);
                std::unique_ptr<Float[]> rgb(new Float[3]);
                for(int i=0;i<3;i++)
                    rgb[i] = p[j].col[i];
                params.AddRGBSpectrum("L", std::move(rgb), 3);
                //AreaLightSource "diffuse"
                pbrt::pbrtAreaLightSource("diffuse", params);
                
                
                //Translate 0 0 10
                pbrt::pbrtTransformBegin();
                pbrt::pbrtTranslate(p[j].pos[0],p[j].pos[1],p[j].pos[2]);
                //std::cout<<p[j].pos[0]<<" "<<p[j].pos[1]<<" "<<p[j].pos[2]<<std::endl;
                //pbrt::pbrtTranslate(0,1,1);
                pbrt::InitParamSet(params, pbrt::SpectrumType::Reflectance);
                //Shape "sphere" "float radius" [1]                
                std::unique_ptr<Float[]> radius(new Float[1]);
                radius[0] = .002;
                params.AddFloat("radius",std::move(radius),1);
                pbrt::pbrtShape("sphere", params);
                pbrt::pbrtTransformEnd();

                //AttributeEnd
                pbrt::pbrtAttributeEnd();
            }
        }      
        
        if(!s.mesh.empty()){

            //    "string type" "fourier" "string bsdffile" "bsdfs/green-acrylic.bsdf"
            pbrt::InitParamSet(params, pbrt::SpectrumType::Reflectance);
            std::unique_ptr<std::string[]> type(new std::string[1]);
            type[0] = "fourier";
            params.AddString("type",std::move(type),1);
            std::unique_ptr<std::string[]> bsdffile(new std::string[1]);
            bsdffile[0] = s.bsdf;
            params.AddString("bsdffile",std::move(bsdffile),1);
            //MakeNamedMaterial "measured_bsdf"
            pbrt::pbrtMakeNamedMaterial("measured_bsdf", params);
         
            //AttributeBegin
            pbrt::pbrtAttributeBegin();
            
            //  Translate 0 -.033 0
            pbrt::pbrtTranslate(s.objectPose[ix][0],s.objectPose[ix][1],s.objectPose[ix][2]);
            //  NamedMaterial "measured_bsdf"
            pbrt::pbrtNamedMaterial("measured_bsdf");
            //  Shape "plymesh" "string filename" "geometry/bunny.ply" 
            pbrt::InitParamSet(params, pbrt::SpectrumType::Reflectance);
            std::unique_ptr<std::string[]> mesh_filename(new std::string[1]);
            mesh_filename[0] = s.mesh;
            params.AddString("filename",std::move(mesh_filename),1);    
            pbrt::pbrtShape("plymesh", params);
            
            //AttributeEnd
            pbrt::pbrtAttributeEnd();
        }

        //WorldEnd
        pbrt::pbrtWorldEnd();
    }
	
    

	
}

static void InitParamSet(ParamSet &ps, SpectrumType type) {
    ps.Clear();
    for (size_t i = 0; i < cur_paramlist.size(); ++i) {
        int type;
        std::string name;
        if (lookupType(cur_paramlist[i].name, &type, name)) {
            if (type == PARAM_TYPE_TEXTURE || type == PARAM_TYPE_STRING ||
                type == PARAM_TYPE_BOOL) {
                if (!cur_paramlist[i].isString) {
                    Error("Expected string parameter value for parameter "
                          "\"%s\" with type \"%s\". Ignoring.",
                          name.c_str(), paramTypeToName(type));
                    continue;
                }
            }
            else if (type != PARAM_TYPE_SPECTRUM) { /* spectrum can be either... */
                if (cur_paramlist[i].isString) {
                    Error("Expected numeric parameter value for parameter "
                          "\"%s\" with type \"%s\".  Ignoring.",
                          name.c_str(), paramTypeToName(type));
                    continue;
                }
            }
            void *data = cur_paramlist[i].arg;
            int nItems = cur_paramlist[i].size;
            if (type == PARAM_TYPE_INT) {
                // parser doesn't handle ints, so convert from doubles here....
                int nAlloc = nItems;
                std::unique_ptr<int[]> idata(new int[nAlloc]);
                double *fdata = (double *)cur_paramlist[i].arg;
                for (int j = 0; j < nAlloc; ++j)
                    idata[j] = int(fdata[j]);
                ps.AddInt(name, std::move(idata), nItems);
            }
            else if (type == PARAM_TYPE_BOOL) {
                // strings -> bools
                int nAlloc = cur_paramlist[i].size;
                std::unique_ptr<bool[]> bdata(new bool[nAlloc]);
                for (int j = 0; j < nAlloc; ++j) {
                    std::string s(((const char **)data)[j]);
                    if (s == "true") bdata[j] = true;
                    else if (s == "false") bdata[j] = false;
                    else {
                        Warning("Value \"%s\" unknown for Boolean parameter \"%s\"."
                            "Using \"false\".", s.c_str(), cur_paramlist[i].name);
                        bdata[j] = false;
                    }
                }
                ps.AddBool(name, std::move(bdata), nItems);
            }
            else if (type == PARAM_TYPE_FLOAT) {
                std::unique_ptr<Float[]> floats(new Float[nItems]);
                for (int i = 0; i < nItems; ++i)
                    floats[i] = ((double *)data)[i];
                ps.AddFloat(name, std::move(floats), nItems);
            } else if (type == PARAM_TYPE_POINT2) {
                if ((nItems % 2) != 0)
                    Warning("Excess values given with point2 parameter \"%s\". "
                            "Ignoring last one of them.", cur_paramlist[i].name);
                std::unique_ptr<Point2f[]> pts(new Point2f[nItems / 2]);
                for (int i = 0; i < nItems / 2; ++i) {
                    pts[i].x = ((double *)data)[2 * i];
                    pts[i].y = ((double *)data)[2 * i + 1];
                }
                ps.AddPoint2f(name, std::move(pts), nItems / 2);
            } else if (type == PARAM_TYPE_VECTOR2) {
                if ((nItems % 2) != 0)
                    Warning("Excess values given with vector2 parameter \"%s\". "
                            "Ignoring last one of them.", cur_paramlist[i].name);
                std::unique_ptr<Vector2f[]> vecs(new Vector2f[nItems / 2]);
                for (int i = 0; i < nItems / 2; ++i) {
                    vecs[i].x = ((double *)data)[2 * i];
                    vecs[i].y = ((double *)data)[2 * i + 1];
                }
                ps.AddVector2f(name, std::move(vecs), nItems / 2);
            } else if (type == PARAM_TYPE_POINT3) {
                if ((nItems % 3) != 0)
                    Warning("Excess values given with point3 parameter \"%s\". "
                            "Ignoring last %d of them.", cur_paramlist[i].name,
                            nItems % 3);
                std::unique_ptr<Point3f[]> pts(new Point3f[nItems / 3]);
                for (int i = 0; i < nItems / 3; ++i) {
                    pts[i].x = ((double *)data)[3 * i];
                    pts[i].y = ((double *)data)[3 * i + 1];
                    pts[i].z = ((double *)data)[3 * i + 2];
                }
                ps.AddPoint3f(name, std::move(pts), nItems / 3);
            } else if (type == PARAM_TYPE_VECTOR3) {
                if ((nItems % 3) != 0)
                    Warning("Excess values given with vector3 parameter \"%s\". "
                            "Ignoring last %d of them.", cur_paramlist[i].name,
                            nItems % 3);
                std::unique_ptr<Vector3f[]> vecs(new Vector3f[nItems / 3]);
                for (int j = 0; j < nItems / 3; ++j) {
                    vecs[j].x = ((double *)data)[3 * j];
                    vecs[j].y = ((double *)data)[3 * j + 1];
                    vecs[j].z = ((double *)data)[3 * j + 2];
                }
                ps.AddVector3f(name, std::move(vecs), nItems / 3);
            } else if (type == PARAM_TYPE_NORMAL) {
                if ((nItems % 3) != 0)
                    Warning("Excess values given with \"normal\" parameter \"%s\". "
                            "Ignoring last %d of them.", cur_paramlist[i].name,
                            nItems % 3);
                std::unique_ptr<Normal3f[]> normals(new Normal3f[nItems / 3]);
                for (int j = 0; j < nItems / 3; ++j) {
                    normals[j].x = ((double *)data)[3 * j];
                    normals[j].y = ((double *)data)[3 * j + 1];
                    normals[j].z = ((double *)data)[3 * j + 2];
                }
                ps.AddNormal3f(name, std::move(normals), nItems / 3);
            } else if (type == PARAM_TYPE_RGB) {
                if ((nItems % 3) != 0) {
                    Warning("Excess RGB values given with parameter \"%s\". "
                            "Ignoring last %d of them", cur_paramlist[i].name,
                            nItems % 3);
                    nItems -= nItems % 3;
                }
                std::unique_ptr<Float[]> floats(new Float[nItems]);
                for (int j = 0; j < nItems; ++j)
                    floats[j] = ((double *)data)[j];
                ps.AddRGBSpectrum(name, std::move(floats), nItems);
            } else if (type == PARAM_TYPE_XYZ) {
                if ((nItems % 3) != 0) {
                    Warning("Excess XYZ values given with parameter \"%s\". "
                            "Ignoring last %d of them", cur_paramlist[i].name,
                            nItems % 3);
                    nItems -= nItems % 3;
                }
                std::unique_ptr<Float[]> floats(new Float[nItems]);
                for (int j = 0; j < nItems; ++j)
                    floats[j] = ((double *)data)[j];
                ps.AddXYZSpectrum(name, std::move(floats), nItems);
            } else if (type == PARAM_TYPE_BLACKBODY) {
                if ((nItems % 2) != 0) {
                    Warning("Excess value given with blackbody parameter \"%s\". "
                            "Ignoring extra one.", cur_paramlist[i].name);
                    nItems -= nItems % 2;
                }
                std::unique_ptr<Float[]> floats(new Float[nItems]);
                for (int j = 0; j < nItems; ++j)
                    floats[j] = ((double *)data)[j];
                ps.AddBlackbodySpectrum(name, std::move(floats), nItems);
            } else if (type == PARAM_TYPE_SPECTRUM) {
                if (cur_paramlist[i].isString) {
                    ps.AddSampledSpectrumFiles(name, (const char **)data, nItems);
                }
                else {
                    if ((nItems % 2) != 0) {
                        Warning("Non-even number of values given with sampled spectrum "
                                "parameter \"%s\". Ignoring extra.",
                                cur_paramlist[i].name);
                        nItems -= nItems % 2;
                    }
                    std::unique_ptr<Float[]> floats(new Float[nItems]);
                    for (int j = 0; j < nItems; ++j)
                        floats[j] = ((double *)data)[j];
                    ps.AddSampledSpectrum(name, std::move(floats), nItems);
                }
            } else if (type == PARAM_TYPE_STRING) {
                std::unique_ptr<std::string[]> strings(new std::string[nItems]);
                for (int j = 0; j < nItems; ++j)
                    strings[j] = std::string(((const char **)data)[j]);
                ps.AddString(name, std::move(strings), nItems);
            }
            else if (type == PARAM_TYPE_TEXTURE) {
                if (nItems == 1) {
                    std::string val(*((const char **)data));
                    ps.AddTexture(name, val);
                }
                else
                    Error("Only one string allowed for \"texture\" parameter \"%s\"",
                          name.c_str());
            }
        }
        else
            Warning("Type of parameter \"%s\" is unknown", cur_paramlist[i].name);
    }
}

static bool lookupType(const char *name, int *type, std::string &sname) {
    CHECK_NOTNULL(name);
    *type = 0;
    const char *strp = name;
    while (*strp && isspace(*strp))
        ++strp;
    if (!*strp) {
        Error("Parameter \"%s\" doesn't have a type declaration?!", name);
        return false;
    }
#define TRY_DECODING_TYPE(name, mask) \
        if (strncmp(name, strp, strlen(name)) == 0) { \
            *type = mask; strp += strlen(name); \
        }
         TRY_DECODING_TYPE("float",     PARAM_TYPE_FLOAT)
    else TRY_DECODING_TYPE("integer",   PARAM_TYPE_INT)
    else TRY_DECODING_TYPE("bool",      PARAM_TYPE_BOOL)
    else TRY_DECODING_TYPE("point2",    PARAM_TYPE_POINT2)
    else TRY_DECODING_TYPE("vector2",   PARAM_TYPE_VECTOR2)
    else TRY_DECODING_TYPE("point3",    PARAM_TYPE_POINT3)
    else TRY_DECODING_TYPE("vector3",   PARAM_TYPE_VECTOR3)
    else TRY_DECODING_TYPE("point",     PARAM_TYPE_POINT3)
    else TRY_DECODING_TYPE("vector",    PARAM_TYPE_VECTOR3)
    else TRY_DECODING_TYPE("normal",    PARAM_TYPE_NORMAL)
    else TRY_DECODING_TYPE("string",    PARAM_TYPE_STRING)
    else TRY_DECODING_TYPE("texture",   PARAM_TYPE_TEXTURE)
    else TRY_DECODING_TYPE("color",     PARAM_TYPE_RGB)
    else TRY_DECODING_TYPE("rgb",       PARAM_TYPE_RGB)
    else TRY_DECODING_TYPE("xyz",       PARAM_TYPE_XYZ)
    else TRY_DECODING_TYPE("blackbody", PARAM_TYPE_BLACKBODY)
    else TRY_DECODING_TYPE("spectrum",  PARAM_TYPE_SPECTRUM)
    else {
        Error("Unable to decode type for name \"%s\"", name);
        return false;
    }
    while (*strp && isspace(*strp))
        ++strp;
    sname = std::string(strp);
    return true;
}

static const char *paramTypeToName(int type) {
    switch (type) {
    case PARAM_TYPE_INT: return "int";
    case PARAM_TYPE_BOOL: return "bool";
    case PARAM_TYPE_FLOAT: return "float";
    case PARAM_TYPE_POINT2: return "point2";
    case PARAM_TYPE_VECTOR2: return "vector2";
    case PARAM_TYPE_POINT3: return "point3";
    case PARAM_TYPE_VECTOR3: return "vector3";
    case PARAM_TYPE_NORMAL: return "normal";
    case PARAM_TYPE_RGB: return "rgb/color";
    case PARAM_TYPE_XYZ: return "xyz";
    case PARAM_TYPE_BLACKBODY: return "blackbody";
    case PARAM_TYPE_SPECTRUM: return "spectrum";
    case PARAM_TYPE_STRING: return "string";
    case PARAM_TYPE_TEXTURE: return "texture";
    default: LOG(FATAL) << "Error in paramTypeToName"; return nullptr;
    }
}

} // namespace pbrt