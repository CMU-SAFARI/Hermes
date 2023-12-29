<p align="center">
  <picture>
  	<source media="(prefers-color-scheme: dark)" srcset="logo/Light.png">
  	<source media="(prefers-color-scheme: light)" srcset="logo/Dark.png">
  <img alt="hermes-logo" src="logo/Dark.png" width="400">
</picture>
  <h3 align="center">Accelerating Long-Latency Load Requests via Perceptron-Based Off-Chip Load Prediction
  </h3>
</p>

<p align="center">
    <a href="https://github.com/CMU-SAFARI/Hermes/blob/master/LICENSE">
        <img alt="GitHub" src="https://img.shields.io/badge/License-MIT-yellow.svg">
    </a>
    <a href="https://github.com/CMU-SAFARI/Hermes/releases">
        <img alt="GitHub release" src="https://img.shields.io/github/release/CMU-SAFARI/Hermes">
    </a>
    <a href="https://doi.org/10.5281/zenodo.6909799"><img src="https://zenodo.org/badge/DOI/10.5281/zenodo.6909799.svg" alt="DOI"></a>
</p>

<details open="open">
  <summary>Table of Contents</summary>
  <ol>
    <li><a href="#what-is-hermes">What is Hermes?</a></li>
    <li><a href="#about-the-framework">About the Framework</a></li>
    <li><a href="#prerequisites">Prerequisites</a></li>
    <li><a href="#installation">Installation</a></li>
    <li><a href="#preparing-traces">Preparing Traces</a></li>
    <li><a href="#experimental-workflow">Experimental Workflow</a></li>
      <ul>
        <li><a href="#launching-experiments">Launching Experiments</a></li>
        <li><a href="#rolling-up-statistics">Rolling up Statistics</a></li>
        <li><a href="#running-mcpat">Running McPAT</a></li>
      </ul>
    </li>
    <li><a href="#brief-code-walkthrough">Brief Code Walkthrough</a></li>
    <li><a href="#frequently-asked-questions">Frequently Asked Questions</a></li>
    <li><a href="#citation">Citation</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>

## What is Hermes?

> Hermes is a speculative mechanism that accelerates long-latency off-chip load requests by removing on-chip cache access latency from their critical path. 

The key idea behind Hermes is to: (1) accurately predict which load requests might go to off-chip, and (2) speculatively start fetching the data required by the predicted off-chip loads directly from the main memory in parallel to the cache accesses. Hermes proposes a lightweight, perceptron-based off-chip predictor that identifies off-chip load requests using multiple disparate program features. The predictor is implemented using only tables and simple arithmetic operations like increment and decrement.

Hermes was presented at MICRO 2022.

> _Rahul Bera, Konstantinos Kanellopoulos, Shankar Balachandran, David Novo, Ataberk Olgun, Mohammad Sadrosadati, Onur Mutlu, "[Hermes: Accelerating Long-Latency Load Requests via Perceptron-Based Off-Chip Load Prediction](https://arxiv.org/pdf/2209.00188.pdf)", In Proceedings of the 55th Annual IEEE/ACM International Symposium on Microarchitecture (MICRO), 2022_

## About The Framework

Hermes is modeled in [ChampSim simulator](https://github.com/ChampSim/ChampSim). This modified simulator version is largely similar to the one used by [Pythia](https://arxiv.org/pdf/2109.12021.pdf) [Bera+, MICRO'21], and fully compatible with all publicly-available traces for ChampSim.

## Prerequisites

The infrastructure has been tested with the following system configuration:

  * cmake 3.20.2
  * gcc v6.3.0
  * perl v5.24.1
  * xz v5.2.5
  * gzip v1.6
  * md5sum v8.26
  * wget v1.18
  * [megatools v1.11.0](https://megatools.megous.com) (Note that v1.9.98 does **NOT** work)

## Installation

0. Install necessary prerequisites
    ```bash
    sudo apt install perl xz gzip
    ```
1. Clone the GitHub repo
   
   ```bash
   git clone https://github.com/CMU-SAFARI/Hermes.git
   ```
2. Set the environment variable as:
   ```bash
    cd Hermes/
    source setvars.sh
   ```
3. Clone the bloomfilter library inside Hermes home directory and build. This should create the static `libbf.a` library inside `build` directory.
   
   ```bash
   cd $HERMES_HOME/
   git clone https://github.com/mavam/libbf.git libbf
   cd libbf/
   mkdir build && cd build/
   cmake ../
   make clean && make -j
   ```
4. Build Hermes using the build script as follows. This should create the executable inside `bin` directory.
   
   ```bash
   cd $HERMES_HOME
   # ./build_champsim.sh <uarch> <l1d_ref> <l2c_pref> <llc_pref> <llc_repl> <ncores> <DRAM-channels> <log-DRAM-channels>
   ./build_champsim.sh glc multi multi multi multi 1 1 0
   ```

   Currently, we support two core microarchitectures: 
    * [glc](https://github.com/CMU-SAFARI/Hermes/blob/main/inc/uarch/glc.h) (modeled after [Intel Goldencove](https://www.anandtech.com/show/16881/a-deep-dive-into-intels-alder-lake-microarchitectures/3))
    * [firestorm](https://github.com/CMU-SAFARI/Hermes/blob/main/inc/uarch/firestorm.h) (modeled after [Apple A14](https://www.anandtech.com/show/16226/apple-silicon-m1-a14-deep-dive/2))

## Preparing Traces
0. Install the megatools executable

    ```bash
    cd $HERMES_HOME/scripts
    wget --no-check-certificate https://megatools.megous.com/builds/megatools-1.11.1.20230212.tar.gz
    tar -xvf megatools-1.11.1.20230212.tar.gz
    ```
> Note: The megatools link might change in the future depending on the latest release. Please recheck the link if the download fails.

1. Use the `download_traces.pl` perl script to download the necessary ChampSim traces used in our paper.

    ```bash
    cd $HERMES_HOME/traces/
    perl $HERMES_HOME/scripts/download_traces.pl --csv artifact_traces.csv --dir ./
    ```
> Note: The script should download **110** traces. Please check the final log for any incomplete downloads. The total size of all traces would be **~36 GB**.

2. Once the trace download completes, please verify the checksum as follows. _Please make sure all traces pass the checksum test._

    ```bash
    cd $HERMES_HOME/traces
    md5sum -c artifact_traces.md5
    ```

3. If the traces are downloaded in some other path, please change the full path in `experiments/MICRO22_AE.tlist` accordingly.

## Experimental Workflow
Our experimental workflow consists of two stages: (1) launching experiments, and (2) rolling up statistics from experiment outputs.

### Launching Experiments
1. To create necessary experiment commands in bulk, we will use `scripts/create_jobfile.pl` script. Please see [scripts/README](https://github.com/CMU-SAFARI/Hermes/blob/main/scripts/README.md) to get a detailed list of supported arguments and their intended use cases.
2. `create_jobfile.pl` requires three necessary arguments:
      * `exe`: the full path of the executable to run
      * `tlist`: contains trace definitions
      * `exp`: contains knobs of the experiments to run
3. Create experiments as follows. _Please make sure the paths used in tlist and exp files are appropriate_.
   
      ```bash
      cd $HERMES_HOME/experiments/
      perl $HERMES_HOME/scripts/create_jobfile.pl --exe $HERMES_HOME/bin/glc-perceptron-no-multi-multi-multi-multi-1core-1ch --tlist MICRO22_AE.tlist --exp MICRO22_AE.exp --local 1 > jobfile.sh
      ```

4. Go to a run directory (or create one) inside `experiments` to launch runs in the following way:
      ```bash
      cd $HERMES_HOME/experiments/outputs/
      source ../jobfile.sh
      ```

5. If you have [slurm](https://slurm.schedmd.com) support to launch multiple jobs in a compute cluster, please provide `--local 0` to `create_jobfile.pl`

### Rolling-up Statistics
1. To rollup stats in bulk, we will use `scripts/rollup.pl` script. Please see [scripts/README](https://github.com/CMU-SAFARI/Hermes/blob/main/scripts/README.md) to get a detailed list of supported arguments and their intended use cases.
2. `rollup.pl` requires three necessary arguments:
      * `tlist`
      * `exp`
      * `mfile`: specifies stat names and reduction method to rollup
3. Rollup statistics as follows. _Please make sure the paths used in tlist and exp files are appropriate_.
   
      ```bash
      cd $HERMES_HOME/experiments/outputs/
      perl ../../scripts/rollup.pl --tlist ../MICRO22_AE.tlist --exp ../rollup_perf_hermes.exp --mfile ../rollup_perf.mfile > rollup.csv
      ```

4. Export the `rollup.csv` file in your favorite data processor (Python Pandas, Excel, Numbers, etc.) to gain insights.

### Running McPAT
McPAT requires an XML file that contains all the necessary statistics (e.g., #L1D hits, #L2C hits, etc.) to compute the runtime dynamic power consumption of the processor. We have already provided a [template XML file](https://github.com/CMU-SAFARI/Hermes/blob/main/mcpat/hermes_glc_template.xml) that models our GLC core configuration. Note that this file is only a template file, meaning we only use a placeholder name for key statistics (e.g., [total cycles](https://github.com/CMU-SAFARI/Hermes/blob/main/mcpat/hermes_glc_template.xml#L33)). 

To generate power consumption stats using McPAT, we need to follow three key steps: (1) replace appropriate statistics from the `.out` file generated by simulator in the template XML file, (2) run McPAT on the generated XML file, and (3) rollup statistics from McPAT output. To automate this process, we have provided some scripts. Please use the following instructions to run them.

1. Checkout McPAT in `mcpat/` directory and compile.
    
    ```bash
    cd $HERMES_HOME/mcpat
    git clone https://github.com/HewlettPackard/mcpat
    cd mcpat/
    git checkout v1.3.0
    make
    ```
2. Now create the jobfile to run the McPAT experiments

    ```bash
    cd $HERMES/experiments
    perl $HERMES_HOME/scripts/create_mcpat_jobfile.pl --exe $HERMES_HOME/scripts/run_mcpat.pl --tlist MICRO22_AE.tlist --exp MICRO22_AE.exp --xmltemplate $HERMES_HOME/mcpat/hermes_glc_template.xml --mcpatexe $HERMES_HOME/mcpat/mcpat/mcpat --statsdir $HERMES_HOME/experiments/outputs/ --outdir $HERMES_HOME/experiments/outputs/ --local 1 > mcpat_jobfile.sh
    ```
    This will essentially create a set of jobs, where each job runs the script `run_mcpat.pl` on a `.out` file generated by the simulator. The script `run_mcpat.pl` does three things: (1) creates a new XML file by by replacing all placeholder stats in the template XML by real all stats from the `.out` file, (2) saves this new XML file in `outdir`, and (3) runs McPAT executable on the generated XML file.

3. Launch the jobs

    ```bash
      cd $HERMES_HOME/experiments/outputs/
      source ../mcpat_jobfile.sh
      ```

4. Once the runs are complete, you can rollup necessary statistics from the McPAT output files using the following script

    ```bash
    cd $HERMES_HOME/experiments/outputs/
    perl ../../scripts/rollup_mcpat.pl --tlist ../MICRO22_AE.tlist --exp ../MICRO22_AE.exp > rollup_mcpat.csv
    ```

    Be careful: the `rollup_mcpat.pl` script is very hardcoded, meaning, it extracts specific stats (e.g., power consumption by dcache) from the McPAT generated output files by grepping and relying on line numbers of the grepped output. This is not the best way to write code. So, if you want to make this code more flexible, please open a pull request and I will be very happy to merge your contribution.

## Brief Code Walkthrough
> Hermes was code-named DDRP (Direct DRAM Prefetch) during development. So any mention of DDRP anywhere in the code inadvertently means Hermes.

- Off-chip prediction mechanism is implemented with an extensible interface in mind. The base off-chip predictor class is defined in `inc/offchip_pred_base.h`.
- There are _nine_ implementations of off-chip predictor shipped out of the box.

  | Predictor type | Description |
  |----------------|-------------|
  | Base | Always NO |
  | Basic | Simple confidence counter-based threshold |
  | Random | Random Hit-miss predictor with a given positive probability |
  | HMP-Local | [Hit-miss predictor](https://ieeexplore.ieee.org/document/765938) [Yoaz+, ISCA'99] with local prediction |
  | HMP-GShare | Hit-miss predictor with GShare prediction |
  | HMP-GSkew | Hit-miss predictor with GSkew prediction |
  | HMP-Ensemble | Hit-miss predictor with all three types combined |
  | TTP | Tag-tracking based predictor |
  | Perc | Perceptron-based OCP used in this paper |

- You can also quickly implement your own off-chip predictor just by extending `OffchipPredBase` class and implement your own `predict()` and `train()` functions. For a new type of off-chip predictor, please call the initialization function in `src/offchip_pred.cc`.
- The off-chip predictor `predict()` function is called at `src/ooo_cpu.cc:1354`, when an LQ entry gets created. The `train()` function is called at `src/ooo_cpu.cc:2281` when an LQ entry gets released.
- Please note that, in the out-of-the-box Hermes configuration, _only_ the memory request that goes out of the LLC is marked as off-chip. If a memory request gets merged on another memory request that has already gone out to off-chip, the waiting memory request will *NOT* be marked as off-chip. This property can be toggled by setting `offchip_pred_mark_merged_load=true`.
- Hermes issues the speculative load requests directly to the main memory controller using the function `issue_ddrp_request()`. This function is only called *after* the translation has been done.

## Frequently Asked Questions

**1. How much memory and timeout should I allocate for each job?**

While most of the experiments to reproduce MICRO'22 key results finish within 4 hours in our Intel(R) Xeon(R) Gold 5118 CPU @ 2.30GHz, some experiments might take considerably higher time to finish (e.g., different experiments with different prefetchers). We suggest putting 12 hours as the timeout and 4 GB as the maximum memory required per job.

**2. Some slurm job failed with the error: *"STEPD TERMINATED DUE TO JOB NOT ENDING WITH SIGNALS"*. What to do?**

This is likely stemming from the slurm scheduler. Please rerun the job either in slurm or in local machine.

**3. Some experiments are not correctly ending. They show the output: _"Reached end of trace for Core: 0 Repeating trace"_... and the error log says _"/usr/bin/xz: Argument list too long_". What to do?**
    
We have encountered this problem sometimes while running jobs in slurm. Please check the xz version in the local machine and rerun the job locally. 

**4. The perl `create_jobfile.pl` script cannot find the Trace module. What to do?**

Have you sourced the `setvars.sh`? The `setvars.sh` script should set the `PERL5LIB` path variable appropriately to make the library discoverable. If the problem still persists, execute the perl script as follows:

```bash
perl -I$HERMES_HOME/scripts $HERMES_HOME/scripts/create_jobfile.pl # the remaining command
```

## Citation
If you use this framework, please cite the following paper:
```
@inproceedings{bera2022,
  author = {Bera, Rahul and Kanellopoulos, Konstantinos and Balachandran, Shankar and Novo, David and Olgun, Ataberk and Sadrosadati, Mohammad and Mutlu, Onur},
  title = {{Hermes: Accelerating Long-Latency Load Requests via Perceptron-Based Off-Chip Load Prediction}},
  booktitle = {Proceedings of the 55th Annual IEEE/ACM International Symposium on Microarchitecture},
  year = {2022}
}
```

## License

Distributed under the MIT License. See `LICENSE` for more information.

## Contact

Rahul Bera - write2bera@gmail.com

## Acknowledgments
We acknowledge support from SAFARI Research Group's industrial partners.
