namespace cli { class settings ; }
namespace pipeline { class pipeline_data; }

namespace player {
  enum run_status {
    run_ok,
    run_fail
  };

  //! \ingroup grp_player
  //! Entry point to running the pipeline.  Deals with a configured pipeline and
  //! adds in the socket and player state.
  run_status run(pipeline::pipeline_data &, const cli::settings &);
}
