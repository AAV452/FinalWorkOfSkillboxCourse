cmake_minimum_required(VERSION 3.17)

project(SkillboxSearchEngine VERSION 0.1) #название и версия проекта (используется в /src)

set(CMAKE_CXX_STANDART 14)

add_subdirectory(nlohmann_json)
add_subdirectory(src)

#для варианта с библиотекой MyLib:
    #создаём библиотеку всех (.срр и .h)-файлов проекта, которую подключим к тестам 
    #в CMakeLists.txt тестов:
    #add_library(MyLib 
    #                    src/mainProject.cpp
    #                    src/myConverterJSON.cpp)
    #target_include_directories(MyLib PUBLIC include) #добавим в библиотеку папку со всеми .h-файлами

#включение тестов и добавление папки с CMakeLists.txt тестов:
enable_testing()
add_subdirectory(tests)