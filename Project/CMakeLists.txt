# ─────────────────────────────────────────────────────────────────────────────
# CMakeLists.txt  —  Hybrid MPI + OpenMP + CUDA password cracker
#
# CUDA is optional: if nvcc is not found the project builds CPU-only.
# Override CUDA architectures with:
#   cmake .. -DCMAKE_CUDA_ARCHITECTURES="75"    (T4  — Colab default)
#   cmake .. -DCMAKE_CUDA_ARCHITECTURES="70"    (V100)
#   cmake .. -DCMAKE_CUDA_ARCHITECTURES="80"    (A100)
#   cmake .. -DCMAKE_CUDA_ARCHITECTURES="70;75;80;86"  (multi-arch)
# ─────────────────────────────────────────────────────────────────────────────
cmake_minimum_required(VERSION 3.18)

# ── Probe for CUDA before declaring the project ───────────────────────────────
include(CheckLanguage)
check_language(CUDA)

if(CMAKE_CUDA_COMPILER)
    project(PasswordCrackerHPC LANGUAGES CXX CUDA)
    set(HAVE_CUDA TRUE)
    message(STATUS "[HPC] CUDA compiler : ${CMAKE_CUDA_COMPILER}")
    # Default to T4 (sm_75); override via -DCMAKE_CUDA_ARCHITECTURES
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES 75)
    endif()
    message(STATUS "[HPC] CUDA arch     : ${CMAKE_CUDA_ARCHITECTURES}")
else()
    project(PasswordCrackerHPC LANGUAGES CXX)
    set(HAVE_CUDA FALSE)
    message(STATUS "[HPC] CUDA not found — building CPU-only (MPI + OpenMP)")
endif()

# ── Language standards ────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(HAVE_CUDA)
    set(CMAKE_CUDA_STANDARD          17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
endif()

# ── Required packages ─────────────────────────────────────────────────────────
find_package(MPI    REQUIRED)
find_package(OpenMP REQUIRED)

# ── Include paths ─────────────────────────────────────────────────────────────
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${MPI_CXX_INCLUDE_PATHS})

# ── Source file lists ─────────────────────────────────────────────────────────
set(CPP_SOURCES
    src/main.cpp
    src/password_generator.cpp
    src/hash_utils.cpp
    src/mpi_master.cpp
    src/mpi_worker.cpp
)

# ── Build target (conditionally includes CUDA sources) ────────────────────────
if(HAVE_CUDA)
    set(CUDA_SOURCES
        cuda/cuda_hash.cu
        cuda/cuda_utils.cu
    )

    add_executable(password_cracker ${CPP_SOURCES} ${CUDA_SOURCES})

    # Expose HAVE_CUDA to all translation units (enables GPU path in mpi_worker)
    target_compile_definitions(password_cracker PRIVATE HAVE_CUDA)

    # CUDA-specific build properties
    set_target_properties(password_cracker PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON      # allows __device__ across TUs
        CUDA_RESOLVE_DEVICE_SYMBOLS   ON
    )

    target_compile_options(password_cracker PRIVATE
        $<$<COMPILE_LANGUAGE:CUDA>:
            -O3
            --use_fast_math
            --ptxas-options=-v
            -Xcompiler -Wall
        >
        $<$<COMPILE_LANGUAGE:CXX>:-Wall -O3>
    )

    target_link_libraries(password_cracker PUBLIC
        MPI::MPI_CXX
        OpenMP::OpenMP_CXX
        cudart
    )

else()
    add_executable(password_cracker ${CPP_SOURCES})

    target_compile_options(password_cracker PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall -O3>
    )

    target_link_libraries(password_cracker PUBLIC
        MPI::MPI_CXX
        OpenMP::OpenMP_CXX
    )
endif()

# ── Installation ──────────────────────────────────────────────────────────────
install(TARGETS password_cracker DESTINATION bin)
