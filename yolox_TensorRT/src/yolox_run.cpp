#include <chrono>
#include <gflags/gflags.h>

#include "../include/yolox.h"

int test_image()
{
    /* Read image */
    const int FLAGS_dump_res = 1;
    const std::string img_path = "/path_to/mmdetection/demo/demo.jpg";
    const std::string engine_file_path = "../yolox_s_sim_modify.engine";
    cv::Mat image = cv::imread(img_path);
    if(image.empty()){
        std::cout<<"Input image path wrong!!"<<std::endl;
        return -1;
    }
    Yolox_Detector* yolox_instance = new Yolox_Detector(engine_file_path);
    
    /* End-to-end infer */
    yolox_instance->do_detection(image);
    
    if(FLAGS_dump_res)
    {
        std::string save_path = "./yolox.jpg";
        cv::imwrite(save_path, image);
    }
    
    if(yolox_instance)
    {
        delete yolox_instance;
        yolox_instance = nullptr;
    }
    return 0;
}

#define WRITE_RES2TXT
void GetFileNames(std::string path, std::vector<std::string>& filenames)
{
    DIR *pDir;
    struct dirent* ptr;
    if(!(pDir = opendir(path.c_str())))
        return;
    while((ptr = readdir(pDir))!=0) {
        if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0)
            filenames.push_back(path + "/" + ptr->d_name);
    }
    closedir(pDir);
}

int test_coco()
{
    const std::string model_path = "../yolox_s_sim_modify.engine";
    Yolox_Detector* yolox_instance = new Yolox_Detector(model_path);
    const std::string cocodataset_path = "/path_to/coco";
    const std::string cocoval_path = cocodataset_path + "/images/val2017";
    const std::string coco_result_dir = "../results";
    std::string res_file_path = "../yolox_coco_eval_res.txt";
    std::vector<std::string> files;
    GetFileNames(cocoval_path, files);

#ifdef WRITE_RES2TXT
    /* prepare result file */
    std::ofstream result_file;
    result_file.open(res_file_path);
    if(!result_file.is_open()){
        std::cout<<"Error:can not create file "<<res_file_path<<std::endl;
        return -2;
    }
#endif

    std::cout<<files.size()<<std::endl;
    auto totaltime = 0;
    int count = 0;
    for (int i = 0; i < files.size(); ++i)
    {
        const std::string input_image_path = files[i];
        std::cout<<"--------------read img---------:"<<input_image_path<<std::endl;
        // std::cout<<i<< ":" <<files[i].c_str()<<std::endl;
        cv::Mat img = cv::imread(input_image_path);
        if(img.empty())
        {
            std::cout<<"Input image path wrong!!"<<std::endl;
            return -1;
        }
        auto start = std::chrono::system_clock::now();
        yolox_instance->do_detection(img);
        auto end = std::chrono::system_clock::now();
        totaltime += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
        std::vector<Object> results;
        results = yolox_instance->getDetectResults();

        cv::Mat dst = img.clone();
        int box_think = (dst.rows + dst.cols) * .001;
        float lable_scale = dst.rows * 0.0009;
        int base_line;
        /* draw & show res  */
        for(const auto&item : results)
        {
            std::string label;
            std::stringstream stream;
            stream <<" cls: "<<item.label<<" prb:"<<item.score << std::endl;
            std::getline(stream,label);
            
            auto size = cv::getTextSize(label , cv::FONT_HERSHEY_COMPLEX , lable_scale , 1 , &base_line);
            
            float x1 = item.rect.x;
            float y1 = item.rect.y;
            float x2 = x1 + item.rect.width;
            float y2 = y1 + item.rect.height;

            cv::rectangle(dst , cv::Point(x1, y1),
                          cv::Point(x2, y2),
                          cv::Scalar(0,0,255) , box_think*2 , 8 , 0);
            cv::putText(dst,label,
                        cv::Point(x2 , y2 - size.height),
                        cv::FONT_HERSHEY_COMPLEX , lable_scale , cv::Scalar(0,255,255) , box_think/2, 8, 0);

#ifdef WRITE_RES2TXT                        
            /* write coco val result to txt file */
            int point_idx       = input_image_path.find(".",0);
            std::string image_id= input_image_path.substr(0,point_idx);
            // std::cout << "image_id: " << image_id << std::endl;
            int category_id     = item.label;
            float x             = x1;
            float y             = y1;
            float w             = item.rect.width;
            float h             = item.rect.height;
            float score         = item.score;
            result_file << image_id << "," << category_id <<","<< x << "," << y
            << "," << w << "," << h <<","<< score << std::endl;
#endif

        }

        int pos = input_image_path.find_last_of('/');
        std::string img_path = coco_result_dir + "/" + input_image_path.substr(pos + 1);
        cv::imwrite(img_path, dst);

        count++;
    }

#ifdef WRITE_RES2TXT
    result_file.close();
#endif

    std::cout<<"Inference Time:"<<totaltime / 5000.<<" ms"<<std::endl;
    std::cout<<"Done!"<<std::endl;
    if(yolox_instance)
    {
        delete yolox_instance;
        yolox_instance = nullptr;
    }
    return 0;
}

int main(int argc, char **argv)
{
    test_coco();
    // test_image();
    return 0;
}