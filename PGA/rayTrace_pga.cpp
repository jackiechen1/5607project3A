//CSCI 5607 HW3 - Rays & Files
//This HW has three steps:
// 1. Compile and run the program (the program takes a single command line argument)
// 2. Understand the code in this file (rayTrace_pga.cpp), in particular be sure to understand:
//     -How ray-sphere intersection works
//     -How the rays are being generated
//     -The pipeline from rays, to intersection, to pixel color
//    After you finish this step, and understand the math, take the HW quiz on canvas
// 3. Update the file parse_pga.h so that the function parseSceneFile() reads the passed in file
//     and sets the relevant global variables for the rest of the code to product to correct image

//To Compile: g++ -fsanitize=address -std=c++11 rayTrace_pga.cpp

//For Visual Studios
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

//Images Lib includes:
#define STB_IMAGE_IMPLEMENTATION //only place once in one .cpp file
#define STB_IMAGE_WRITE_IMPLEMENTATION //only place once in one .cpp files
#define M_PI 3.1415926
#include "image_lib.h" //Defines an image class and a color class

//#3D PGA
#include "PGA_3D.h"

//High resolution timer
#include <chrono>

//Scene file parser
#include "parse_pga.h"


bool raySphereIntersect_fast(Point3D rayStart, Line3D rayLine, Point3D sphereCenter, float sphereRadius){
  Dir3D dir = rayLine.dir();
  float a = dot(dir,dir);
  Dir3D toStart = (rayStart - sphereCenter);
  float b = 2 * dot(dir,toStart);
  float c = dot(toStart,toStart) - sphereRadius*sphereRadius;
  float discr = b*b - 4*a*c;
  if (discr < 0) return false;
  else{
    float t0 = (-b + sqrt(discr))/(2*a);
    float t1 = (-b - sqrt(discr))/(2*a);
    if (t0 > 0 || t1 > 0) return true;
  }
  return false;
}

bool raySphereIntersect(Point3D rayStart, Line3D rayLine, Point3D sphereCenter, float sphereRadius){
  Point3D projPoint = dot(rayLine,sphereCenter)*rayLine;      //Project to find closest point between circle center and line [proj(sphereCenter,rayLine);]
  float distSqr = projPoint.distToSqr(sphereCenter);          //Point-line distance (squared)
  float d2 = distSqr/(sphereRadius*sphereRadius);             //If distance is larger than radius, then...
  if (d2 > 1) return false;                                   //... the ray missed the sphere
  float w = sphereRadius*sqrt(1-d2);                          //Pythagorean theorem to determine dist between proj point and intersection points
  Point3D p1 = projPoint + rayLine.dir()*w;                   //Add/subtract above distance to find hit points
  Point3D p2 = projPoint - rayLine.dir()*w; 

  if (dot((p2 - rayStart), rayLine.dir()) >= 0)
  {
      return true;     //Is the second point in same direction as the ray line?
  }
  if (dot((p1 - rayStart), rayLine.dir()) >= 0)
  {
      return true;     //Is the first point in same direction as the ray line?
  }
  
  return false;
}
Hit lightSphereIntersect(Point3D rayStart, Line3D rayLine, Point3D sphereCenter, float sphereRadius,Point3D lightSource) {
    Point3D projPoint = dot(rayLine, sphereCenter) * rayLine;      //Project to find closest point between circle center and line [proj(sphereCenter,rayLine);]
    float distSqr = projPoint.distToSqr(sphereCenter);          //Point-line distance (squared)
    float d2 = distSqr / (sphereRadius * sphereRadius);             //If distance is larger than radius, then...
    
    float w = sphereRadius * sqrt(1 - d2);                          //Pythagorean theorem to determine dist between proj point and intersection points
    Point3D p1 = projPoint + rayLine.dir() * w;                   //Add/subtract above distance to find hit points
    Point3D p2 = projPoint - rayLine.dir() * w;
    
    if (dot((p2 - rayStart), rayLine.dir()) >= 0)
    {
        // p2 is the interact point
        Dir3D N = p2 - sphereCenter;
        N.normalized();
        Dir3D V = rayStart - p2;
        V.normalized();
        Dir3D L = lightSource - p2;
        float intensity = L.magnitude();
        intensity = 1 / (0.25 * intensity);
        L.normalized();
        Dir3D R = L - 2 * (dot(L, N) * N);
        R.normalized();
        
        return Hit(N, V, R, L,intensity,p2);
    }
    
}

Color GetColor(Material material, Dir3D N, Dir3D L, Dir3D V, int Sl, float intensity, Dir3D R); // sum over point light term
Color AddColor(Color color, Color emissive, Material mat, Color ambient); // add on ambient term
int main(int argc, char** argv){
  
  //Read command line paramaters to get scene file
  if (argc != 2){
     std::cout << "Usage: ./a.out scenefile\n";
     return(0);
  }
  std::string secenFileName = argv[1];

  //Parse Scene File
  parseSceneFile(secenFileName);

  float imgW = img_width, imgH = img_height;
  float halfW = imgW/2, halfH = imgH/2;
  float d = halfH / tanf(halfAngleVFOV * (M_PI / 180.0f));
  // d = halfW / tanf(halfAngleVFOV * (M_PI / 180.0f));
  Color back = backgroundcolor;
  Image outputImg = Image(img_width,img_height);
  auto t_start = std::chrono::high_resolution_clock::now();


  for (int i = 0; i < img_width; i++){
    for (int j = 0; j < img_height; j++){
      //TODO: In what way does this assumes the basis is orthonormal?
        float u,v;
        if (sampling == 0) // jittered supersampling
        {
            float rand1 = (float)rand() / (RAND_MAX);
            float rand2 = (float)rand() / (RAND_MAX);
            u = (halfW - (imgW) * ((i + rand1) / imgW));
            v = (halfH - (imgH) * ((j + rand2) / imgH));
        }
        else if (sampling == 1) //adaptive supersampling
        {

        }
        else// basic sampling
        {
            u = (halfW - (imgW) * ((i + 0.5) / imgW));
            v = (halfH - (imgH) * ((j + 0.5) / imgH));
        }
      
      Point3D p = eye - d*forward + u*right + v*up;
      Dir3D rayDir = (p - eye); 
      Line3D rayLine = vee(eye,rayDir).normalized();  //Normalizing here is optional
      Color color = back;
      float shortest = INT_MAX;
      for (int k = 0; k < scene.spheres.size(); k++) // traverse all object in the scene
      {
          bool hit = raySphereIntersect(eye, rayLine, scene.spheres[k].pos, scene.spheres[k].radius);
          if(hit)
          {
              Dir3D view = eye - scene.spheres[k].pos;
              float distance = view.magnitude();
              if (distance < shortest) //find the spheres closest to viewer
              {
                  shortest = distance;
                  // add ambient response
                  color = AddColor(Color(0, 0, 0), Color(0, 0, 0), scene.spheres[k].material, ambient); // add ambient light
                  
                  // add point light
                  for (int l = 0; l < scene.pointLights.size(); l++) // sum over all point light
                  {
                      int shadow = 1;
                      Hit interact = lightSphereIntersect(eye, rayLine, scene.spheres[k].pos, scene.spheres[k].radius, scene.pointLights[l].center);
                      for (int ki = 0; ki < scene.spheres.size(); ki++) // this loop check the shadow, if the light is visible, not block by others
                      {
                          if (ki != k) // don't check itself
                          {
                              Dir3D rayDir1 = (scene.pointLights[l].center - interact.interacts);
                              Line3D rayLine1 = vee(interact.interacts, rayDir1).normalized();
                              if (raySphereIntersect(interact.interacts, rayLine1, scene.spheres[ki].pos, scene.spheres[ki].radius)) // if hit, generate shadow
                              {
                                  //std::cout << "shadow 0" << std::endl;
                                  shadow = 0;
                              }
                          }
                      }
                      color = color + GetColor(scene.spheres[k].material, interact.N, interact.L, interact.V, shadow, interact.intensity, interact.R); //Sl ??                     
                  }
                  
              }
          }
         
      }
      
      outputImg.setPixel(i, j, color);
      //outputImg.setPixel(i,j, Color(fabs(i/imgW),fabs(j/imgH),fabs(0))); //TODO: Try this, what is it visualizing?
    }
  }
  auto t_end = std::chrono::high_resolution_clock::now();
  printf("Rendering took %.2f ms\n",std::chrono::duration<double, std::milli>(t_end-t_start).count());

  outputImg.write(imgName.c_str());
  return 0;
}
Color GetColor(Material material, Dir3D N, Dir3D L, Dir3D V, int Sl, float intensity, Dir3D R)
{
    float parameter1 = dot(N, L);
    if (parameter1 < 0)
    {
        parameter1 = 0;
    }
    float parameter2 = pow(dot(V, R), material.ns);
    float r = (material.dr * parameter1 + material.sr * parameter2) * Sl * intensity;
    float g = (material.dg * parameter1 + material.sg * parameter2) * Sl * intensity;
    float b = (material.db * parameter1 + material.sb * parameter2) * Sl * intensity;
    return Color(r, g, b);
}
Color AddColor(Color color, Color emissive, Material mat, Color ambient)
{
    float r = color.r + emissive.r + mat.ar*ambient.r;
    float g = color.g + emissive.g + mat.ag*ambient.g;
    float b = color.b + emissive.b + mat.ab*ambient.b;
    return Color(r, g, b);
}
/*
Color EvaluateRayTree(Scene scene, Ray ray) {
    bool hit_somthing;
    HitInformation Hit; // structure containing hit point, normal, etc
    hit_something = FindIntersection(scene, ray, &Hit);
    if (hit_something) {
        return ApplyLightingModel(ray, hit);
    }
    else {
        return BackgroundColor;
    }
}
Color ApplyLightingModel(Color back, Scene scene, Ray ray, HitInformation hit) {
    Color contribution = back;
    for (int l = 0; l < scene.pointLights.size(); l++) // sum over all point light
    {
        Ray shadow(hit.interacts, scene.pointLights[l].center ¨C hit.interacts);
        HitInformation shadow_hit;
        bool blocked = FindIntersection(scene, shadow, &shadow_hit);
        if (blocked && shadow_hit.t < Distance(L.pos, hit.pos)) {
            continue; // we¡¯re in shadow, on to the next light;
        }
        contribution += DiffuseContribution(L, hit);
        contribution += SpecularContribtion(L, ray, hit);
    }
    Ray mirror = Reflect(ray, hit.normal);
    contribution += Kr * EvaluateRayTree(scene, mirror);
    Ray glass = Refract(ray, hit.normal);
    contribution += Kt * EvaluateRayTree(scene, glass);
    contribution += Ambient(); // superhack!
    contribution += Emission(hit); // for area light sources only
}
DiffuseContribution(L, hit);
SpecularContribtion(L, ray, hit);
Reflect(ray, hit.normal);
Refract(ray, hit.normal);

*/