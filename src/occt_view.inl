
// Member function to delete variable arguments
template <typename... Args> void Occt_view::remove(Args&&... args)
{
  for_each_flat(
      [&](AIS_Shape_ptr& s)
      {
        m_ctx->Remove(s, true);
      },
      std::forward<Args>(args)...);
}

template <typename Shp_ptr_t, typename T>
void show(AIS_InteractiveContext& ctx, Shp_ptr_t& shp, const T& obj, bool redraw)
{
  if (shp)
  {
    shp->Set(obj);
    ctx.Redisplay(shp, redraw);
  }
  else
  {
    shp = new Shp_ptr_t::element_type(obj);
    ctx.Display(shp, redraw);
  }
}

template <typename Shp_ptr_t, typename T>
void show(AIS_InteractiveContext& ctx, Sketch& owner, Shp_ptr_t& shp, const T& obj, bool redraw)
{
  if (shp)
  {
    shp->Set(obj);
    ctx.Redisplay(shp, redraw);
  }
  else
  {
    shp = new Shp_ptr_t::element_type(owner, obj);
    ctx.Display(shp, redraw);
  }
}