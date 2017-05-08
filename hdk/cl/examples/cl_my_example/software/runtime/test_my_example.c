// Amazon FPGA Hardware Development Kit
//
// Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Amazon Software License (the "License"). You may not use
// this file except in compliance with the License. A copy of the License is
// located at
//
//    http://aws.amazon.com/asl/
//
// or in the "license" file accompanying this file. This file is distributed on
// an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express or
// implied. See the License for the specific language governing permissions and
// limitations under the License.


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fpga_pci.h>
#include <fpga_mgmt.h>
#include <utils/lcd.h>

/* Constants determined by the CL */
/* a set of register offsets; this CL has only one */
/* these register addresses should match the addresses in */
/* /aws-fpga/hdk/cl/examples/common/cl_common_defines.vh */

#define MY_EXAMPLE_REG_ADDR UINT64_C(0x500)
#define VLED_REG_ADDR	UINT64_C(0x504)

/*
 * pci_vendor_id and pci_device_id values below are Amazon's and avaliable to use for a given FPGA slot. 
 * Users may replace these with their own if allocated to them by PCI SIG
 */
static uint16_t pci_vendor_id = 0x1D0F; /* Amazon PCI Vendor ID */
static uint16_t pci_device_id = 0xF000; /* PCI Device ID preassigned by Amazon for F1 applications */


/* use the stdout logger for printing debug information  */
const struct logger *logger = &logger_stdout;

/* Declaring the local functions */

int get_dot_product_fpga(int slot, int pf_id, int bar_id, uint16_t *vec1, uint16_t *vec2, uint32_t vec_size, double expect);
double get_elapsed_time(time_t end, time_t start);
/* Declating auxilary house keeping functions */
int initialize_log(char* log_name);
int check_afi_ready(int slot);


int main(int argc, char **argv) {
    int rc;
    int slot_id;

    /* initialize the fpga_pci library so we could have access to FPGA PCIe from this applications */
    rc = fpga_pci_init();
    fail_on(rc, out, "Unable to initialize the fpga_pci library");

    /* This demo works with single FPGA slot, we pick slot #0 as it works for both f1.2xl and f1.16xl */

    slot_id = 0;

    rc = check_afi_ready(slot_id);
    fail_on(rc, out, "AFI not ready");
    
    /* Accessing the CL registers via AppPF BAR0, which maps to sh_cl_ocl_ AXI-Lite bus between AWS FPGA Shell and the CL*/

    uint32_t num_iters = 5000;
    uint32_t vec_size = 1000;
    printf("===== Starting computation of %d dimension vector inner product %d times=====\n", vec_size, num_iters);   
    uint32_t k;
    struct timeval stop, start;
    gettimeofday(&start, NULL);
    for (k = 0; k < num_iters; ++k) {
    	uint16_t *vec1 = (uint16_t *) malloc(vec_size * sizeof(uint16_t));
    	uint16_t *vec2 = (uint16_t *) malloc(vec_size * sizeof(uint16_t));

    	uint32_t i = 0;
    	
        double result = 0;
    	for (i = 0; i < vec_size; ++i) {
    	    vec1[i] = rand();
	    vec2[i] = rand();
    	}	
    	
    	
	for (i = 0; i < vec_size; ++i) {
	    result += ((uint32_t)vec1[i]) * ((uint32_t)vec2[i]);
    	}
   	
	if (k == 0) { 
    	    rc = get_dot_product_fpga(slot_id, FPGA_APP_PF, APP_PF_BAR0, vec1, vec2, vec_size, result);
	    fail_on(rc, out, "fpga dot product failed");    

	}
	
	free(vec1);
    	free(vec2);
    }
    gettimeofday(&stop, NULL);
    printf("Runtime of CPI is %lu milliseconds\n", stop.tv_usec - start.tv_usec);
    
    return rc;
out:
    return 1;
}

double get_cpu_time(clock_t end, clock_t start) {
    return ((double) (end-start) * 1000)/CLOCKS_PER_SEC;
}

double get_elapsed_time(time_t end, time_t start) {
    return difftime(end, start) * 1000;
}
/* use FPGA to calculate dot product
 */
int get_dot_product_fpga(int slot_id, int pf_id, int bar_id, uint16_t *vec1, uint16_t *vec2, uint32_t vec_size, double expect) {
    int rc;
    /* pci_bar_handle_t is a handler for an address space exposed by one PCI BAR on one of the PCI PFs of the FPGA */

    pci_bar_handle_t pci_bar_handle = PCI_BAR_HANDLE_INIT;

    /* attach to the fpga, with a pci_bar_handle out param
     * To attach to multiple slots or BARs, call this function multiple times,
     * saving the pci_bar_handle to specify which address space to interact with in
     * other API calls.
     * This function accepts the slot_id, physical function, and bar number
     */
    rc = fpga_pci_attach(slot_id, pf_id, bar_id, 0, &pci_bar_handle);
    fail_on(rc, out, "Unable to attach to the AFI on slot id %d", slot_id);

    double result = 0;
    uint32_t i = 0;

    struct timeval stop, start;
    gettimeofday(&start, NULL);

    for (i = 0; i < vec_size; ++i) {
    	/* write a value into the mapped address space */
	uint32_t value = (vec1[i] << 16) + vec2[i]; 
	rc = fpga_pci_poke(pci_bar_handle, MY_EXAMPLE_REG_ADDR, value);
    	fail_on(rc, out, "Unable to write to the fpga !");

    	/* read it back as a product of the lower and upper 16 bit unsigned */
        
    	rc = fpga_pci_peek(pci_bar_handle, MY_EXAMPLE_REG_ADDR, &value);

    	fail_on(rc, out, "Unable to read read from the fpga !");
        result += value;
    }


    if (result != expect) {
	printf("result of FPGA %.2f does not match expected %.2f\n", result, expect);
    }
    else {
	printf("result of FPGA and CPU match!\n");
    }

    gettimeofday(&stop, NULL);
    printf("Runtime of FPGA is %lu milliseconds\n", stop.tv_usec - start.tv_usec);
out:
    /* clean up */
    if (pci_bar_handle >= 0) {
        rc = fpga_pci_detach(pci_bar_handle);
        if (rc) {
            printf("Failure while detaching from the fpga.\n");
        }
    }

    /* if there is an error code, exit with status 1 */
    return (rc != 0 ? 1 : 0);
}


/*
 * check if the corresponding AFI for my_example is loaded
 */

int check_afi_ready(int slot_id) {
    struct fpga_mgmt_image_info info = {0}; 
    int rc;

    /* get local image description, contains status, vendor id, and device id. */
    rc = fpga_mgmt_describe_local_image(slot_id, &info,0);
    fail_on(rc, out, "Unable to get AFI information from slot %d. Are you running as root?",slot_id);

    /* check to see if the slot is ready */
    if (info.status != FPGA_STATUS_LOADED) {
        rc = 1;
        fail_on(rc, out, "AFI in Slot %d is not in READY state !", slot_id);
    }

    printf("AFI PCI  Vendor ID: 0x%x, Device ID 0x%x\n",
        info.spec.map[FPGA_APP_PF].vendor_id,
        info.spec.map[FPGA_APP_PF].device_id);

    /* confirm that the AFI that we expect is in fact loaded */
    if (info.spec.map[FPGA_APP_PF].vendor_id != pci_vendor_id ||
        info.spec.map[FPGA_APP_PF].device_id != pci_device_id) {
        printf("AFI does not show expected PCI vendor id and device ID. If the AFI "
               "was just loaded, it might need a rescan. Rescanning now.\n");

        rc = fpga_pci_rescan_slot_app_pfs(slot_id);
        fail_on(rc, out, "Unable to update PF for slot %d",slot_id);
        /* get local image description, contains status, vendor id, and device id. */
        rc = fpga_mgmt_describe_local_image(slot_id, &info,0);
        fail_on(rc, out, "Unable to get AFI information from slot %d",slot_id);

        printf("AFI PCI  Vendor ID: 0x%x, Device ID 0x%x\n",
            info.spec.map[FPGA_APP_PF].vendor_id,
            info.spec.map[FPGA_APP_PF].device_id);

        /* confirm that the AFI that we expect is in fact loaded after rescan */
        if (info.spec.map[FPGA_APP_PF].vendor_id != pci_vendor_id ||
             info.spec.map[FPGA_APP_PF].device_id != pci_device_id) {
            rc = 1;
            fail_on(rc, out, "The PCI vendor id and device of the loaded AFI are not "
                             "the expected values.");
        }
    }
    
    return rc;

out:
    return 1;
}
