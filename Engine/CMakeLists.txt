# 配置SDL为静态库
set(SDL_SHARED FALSE)
set(SDL_STATIC TRUE)
add_subdirectory(SDL)
add_subdirectory(entt)