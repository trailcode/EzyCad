// Base template for vectors (not a specialization of Safe_container)
template <typename Vec, typename Scalar, int Components>
class Vector_container
{
 private:
  Vec m_value;

 public:
  explicit Vector_container(Vec initial_value)
      : m_value(initial_value) {}
  Vec& unsafe_get()
  {
    return m_value;
  }
  const Vec& unsafe_get() const
  {
    return m_value;
  }
  Scalar& unsafe_get_x()
  {
    return m_value.x;
  }
  const Scalar& unsafe_get_x() const
  {
    return m_value.x;
  }
  Scalar& unsafe_get_y()
  {
    return m_value.y;
  }
  const Scalar& unsafe_get_y() const
  {
    return m_value.y;
  }
  Scalar& unsafe_get_z()
    requires(Components >= 3)
  {
    return m_value.z;
  }
  const Scalar& unsafe_get_z() const
    requires(Components >= 3)
  {
    return m_value.z;
  }
  Scalar& unsafe_get_w()
    requires(Components >= 4)
  {
    return m_value.w;
  }
  const Scalar& unsafe_get_w() const
    requires(Components >= 4)
  {
    return m_value.w;
  }
};

// Explicit specializations for GLM vector types
template <> class SafeType<glm::ivec2> : public Vector_container<glm::ivec2, int, 2>
{
 public:
  explicit SafeType(glm::ivec2 initial_value)
      : Vector_container<glm::ivec2, int, 2>(initial_value) {}
};
template <> class SafeType<glm::fvec2> : public Vector_container<glm::fvec2, float, 2>
{
 public:
  explicit SafeType(glm::fvec2 initial_value)
      : Vector_container<glm::fvec2, float, 2>(initial_value) {}
};
template <> class SafeType<glm::dvec2> : public Vector_container<glm::dvec2, double, 2>
{
 public:
  explicit SafeType(glm::dvec2 initial_value)
      : Vector_container<glm::dvec2, double, 2>(initial_value) {}
};
template <> class SafeType<glm::ivec3> : public Vector_container<glm::ivec3, int, 3>
{
 public:
  explicit SafeType(glm::ivec3 initial_value)
      : Vector_container<glm::ivec3, int, 3>(initial_value) {}
};
template <> class SafeType<glm::fvec3> : public Vector_container<glm::fvec3, float, 3>
{
 public:
  explicit SafeType(glm::fvec3 initial_value)
      : Vector_container<glm::fvec3, float, 3>(initial_value) {}
};
template <> class SafeType<glm::dvec3> : public Vector_container<glm::dvec3, double, 3>
{
 public:
  explicit SafeType(glm::dvec3 initial_value)
      : Vector_container<glm::dvec3, double, 3>(initial_value) {}
};
template <> class SafeType<glm::ivec4> : public Vector_container<glm::ivec4, int, 4>
{
 public:
  explicit SafeType(glm::ivec4 initial_value)
      : Vector_container<glm::ivec4, int, 4>(initial_value) {}
};
template <> class SafeType<glm::fvec4> : public Vector_container<glm::fvec4, float, 4>
{
 public:
  explicit SafeType(glm::fvec4 initial_value)
      : Vector_container<glm::fvec4, float, 4>(initial_value) {}
};
template <> class SafeType<glm::dvec4> : public Vector_container<glm::dvec4, double, 4>
{
 public:
  explicit SafeType(glm::dvec4 initial_value)
      : Vector_container<glm::dvec4, double, 4>(initial_value) {}
};
