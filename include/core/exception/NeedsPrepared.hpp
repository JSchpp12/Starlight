// #pragma once

// #include <exception>

// namespace star::core::exception
// {
// class NeedsPrepared : public std::exception
// {
//   public:
//     /// @brief Signals when a renderable resource has not been properly prepared. For example, a pipline might throw
//     /// this exception if the pipeline has not been built
//     NeedsPrepared() noexcept : std::exception() {};
//     explicit NeedsPrepared(const char *message) : std::exception(message) {};
// };
// } // namespace star::core::exception