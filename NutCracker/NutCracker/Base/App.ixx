export import <span>;
export import <tuple>;
import <ctime>;

#include <Core/Logging.hxx>

export module NutCracker.Base.App;

namespace NutCracker
{
	export 
	class BaseInstance { // singleton
	private:
		inline static BaseInstance* s_TheAppInstance = nullptr;

	public:
		BaseInstance () {
			if (s_TheAppInstance == nullptr)
				s_TheAppInstance = this;
			else THROW_CORE_critical("Application Instance already running");
		}
		inline static BaseInstance* GetSingleton () { if (s_TheAppInstance != nullptr) return s_TheAppInstance; else THROW_CORE_critical("No Application created/instantiated"); }
		virtual ~BaseInstance() = default;

	public:
		inline static std::pair<float, float> UpdateTime() {
			clock_t time_now = clock();
			std::pair<float, float> update_and_render_latency {
				/*.first  =*/ float(double(time_now - s_TheAppInstance->m_UpdateTimestamp) / CLOCKS_PER_SEC),
				/*.second =*/ float(double(time_now - s_TheAppInstance->m_RenderTimestamp) / CLOCKS_PER_SEC)
			};
			s_TheAppInstance->m_UpdateTimestamp = time_now;
			s_TheAppInstance->m_RenderTimestamp = time_now;
			return update_and_render_latency;
		};
		
	public: // public interface
		inline static void Setup (const std::span<char*> &argument_list) { s_TheAppInstance->setup (argument_list); }
		inline static void InitializeVk () { s_TheAppInstance->initializeVk (); }
		inline static void Run () {
			UpdateTime ();
			while (s_TheAppInstance->keep_context_running ()) {
				auto [update_latency, render_latency] = UpdateTime ();
				s_TheAppInstance->update (update_latency), s_TheAppInstance->render(render_latency);
			}
		}
		inline static void TerminateVk () { s_TheAppInstance->terminateVk(); } 
		inline static void Cleanup () { s_TheAppInstance->cleanup(); } 

		inline static const double GetRenderTimestamp() { return double(s_TheAppInstance->m_RenderTimestamp) / CLOCKS_PER_SEC; }
		inline static const double GetUpdateTimestamp() { return double(s_TheAppInstance->m_UpdateTimestamp) / CLOCKS_PER_SEC; }

	protected: // required defined
		virtual void setup (const std::span<char*> &argument_list) = 0;
		virtual void initializeVk () = 0;
		virtual void update (double update_latency) = 0;
		virtual void render (double render_latency) = 0;
		virtual void terminateVk () = 0;
		virtual void cleanup () = 0;
		virtual void on_event () = 0;
		virtual bool keep_context_running () = 0;
	private:
		clock_t m_UpdateTimestamp = clock(), m_RenderTimestamp = clock();
	};
};