import NutCracker.Base.Window;

#if NTKR_WINDOWS
import NutCracker.Windows.WindowImpl;
#else
static_assert (false, "platform unsupported");
#endif

// NutCracker::Window* Create (const NutCracker::WindowProps& props) 
NutCracker::Window* NutCracker::Window::Create (NutCracker::WindowProps const & props)
{
	return new NutCracker::WindowImpl (props);
}